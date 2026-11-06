#include <iostream>
// miniaudio is a single header. Defining this here (and only here) makes this
// file compile the library itself; everywhere else it would just be declarations
#define MINIAUDIO_IMPLEMENTATION
#include "miniaudio.h"
#include "Audio.h"

namespace
{
	// Copies of each effect, so the same sound can play several times at once
	// (four bombs exploding together)
	constexpr int VOICES = 4;
	// The same effect isn't started twice in less than this. In a chain of 6 bombs
	// they all explode in the same frame, and 6 explosions at once would just be loud
	constexpr ma_uint64 MIN_REPEAT_MS = 40;
	constexpr float MUSIC_VOLUME = 0.45f; // music volume

    struct SoundInfo
    {
        const char *file;
        float volume;
    };

    // One entry per Sound, in the same order as the enum. Each one has its own
    // volume, so a soft sound like the bomb tick doesn't cover everything
    const SoundInfo SOUNDS[] =
    {
        { "sounds/bomb_place.wav", 0.8f },
        { "sounds/bomb_tick.wav", 0.3f },
        { "sounds/explosion.wav", 0.9f },
        { "sounds/bush_break.wav", 0.7f },
        { "sounds/item_get.wav", 0.8f },
        { "sounds/jump.wav", 0.6f },
        { "sounds/land.wav", 0.4f },
        { "sounds/player_death.wav", 0.9f },
        { "sounds/heart_lost.wav", 0.9f },
        { "sounds/enemy_death.wav", 0.8f },
        { "sounds/boss_hit.wav", 0.9f },
        { "sounds/boss_attack.wav", 0.7f },
        { "sounds/door_open.wav", 0.8f },
        { "sounds/time_warning.wav", 0.8f },
        { "sounds/menu_move.wav", 0.6f },
        { "sounds/menu_select.wav", 0.7f }
    };

    // Same order as the Music enum (NONE has no file)
    const char *MUSIC_FILES[] =
    {
        nullptr,
        "sounds/music_title.mp3",
        "sounds/music_map.mp3",
        "sounds/music_stage1.mp3",
        "sounds/music_stage2.mp3",
        "sounds/music_stage3.mp3",
        "sounds/music_stage4.mp3",
        "sounds/music_boss.mp3",
        "sounds/jingle_clear.mp3",
        "sounds/jingle_timeup.mp3",
        "sounds/jingle_gameover.mp3",
        "sounds/jingle_victory.mp3"
    };
}

// Everything that needs miniaudio's types lives here, inside the .cpp. Audio.h
// only sees a pointer to it, so no other file has to include miniaudio.h
struct Audio::Impl
{
    struct Effect
    {
        ma_sound voices[VOICES];
        // How many voices loaded correctly (0 = the file is missing)
        int voiceCount = 0;
        int nextVoice = 0;
        bool played = false;
        ma_uint64 lastPlayMs = 0;
    };

    ma_engine engine;
    bool engineReady = false;

    Effect effects[int(Sound::COUNT)];

    ma_sound music;
    bool musicLoaded = false;
    Music currentMusic = Music::NONE;
};

Audio::Audio()
    : impl(new Impl())
{
}

Audio::~Audio()
{
    shutdown();
}

void Audio::init()
{
    if (impl->engineReady)
        return;

    // The engine opens the default audio device. If there isn't one, we just
    // stay silent: every play function checks engineReady first
    if (ma_engine_init(NULL, &impl->engine) != MA_SUCCESS)
    {
        std::cout << "Could not start the audio, the game will be silent" << std::endl;
        return;
    }
    impl->engineReady = true;

    for (int i = 0; i < int(Sound::COUNT); ++i)
    {
        Impl::Effect &effect = impl->effects[i];

        // Effects are decoded once at the start, so playing them later is instant.
        // The first voice decodes the file and the others share its data
        if (ma_sound_init_from_file(&impl->engine, SOUNDS[i].file, MA_SOUND_FLAG_DECODE,
                                    NULL, NULL, &effect.voices[0]) != MA_SUCCESS)
        {
            std::cout << "Could not load " << SOUNDS[i].file << std::endl;
            continue;
        }

        effect.voiceCount = 1;
        while (effect.voiceCount < VOICES &&
               ma_sound_init_copy(&impl->engine, &effect.voices[0], 0, NULL,
                                  &effect.voices[effect.voiceCount]) == MA_SUCCESS)
            ++effect.voiceCount;

        for (int v = 0; v < effect.voiceCount; ++v)
            ma_sound_set_volume(&effect.voices[v], SOUNDS[i].volume);
    }
}

void Audio::shutdown()
{
    if (!impl->engineReady)
        return;

    stopMusic();

    for (Impl::Effect &effect : impl->effects)
    {
        // Copies first, the original sound owns the decoded data
        for (int v = effect.voiceCount - 1; v >= 0; --v)
            ma_sound_uninit(&effect.voices[v]);
        effect.voiceCount = 0;
    }

    ma_engine_uninit(&impl->engine);
    impl->engineReady = false;
}

void Audio::playSound(Sound sound)
{
    if (!impl->engineReady)
        return;

    Impl::Effect &effect = impl->effects[int(sound)];
    if (effect.voiceCount == 0)
        return;

    // Skip it if this same effect started a moment ago
    const ma_uint64 now = ma_engine_get_time_in_milliseconds(&impl->engine);
    if (effect.played && now - effect.lastPlayMs < MIN_REPEAT_MS)
        return;
    effect.played = true;
    effect.lastPlayMs = now;

    // Use a free voice, or restart the oldest one if all of them are playing
    ma_sound *voice = nullptr;
    for (int v = 0; v < effect.voiceCount && voice == nullptr; ++v)
        if (!ma_sound_is_playing(&effect.voices[v]))
            voice = &effect.voices[v];
    if (voice == nullptr)
    {
        voice = &effect.voices[effect.nextVoice];
        effect.nextVoice = (effect.nextVoice + 1) % effect.voiceCount;
    }

    // From the beginning, in case this voice had already been used
    ma_sound_seek_to_pcm_frame(voice, 0);
    ma_sound_start(voice);
}

void Audio::playMusic(Music music, bool loop)
{
    if (!impl->engineReady)
        return;

    // The same song is already on (for example, the menu music continuing after the intro)
    if (music == impl->currentMusic && impl->musicLoaded && ma_sound_is_playing(&impl->music))
        return;

    stopMusic();
    if (music == Music::NONE)
        return;

    // Music is streamed from the file instead of being decoded all at once,
    // because a few minutes of decoded music would take a lot of memory
    const char *file = MUSIC_FILES[int(music)];
    if (ma_sound_init_from_file(&impl->engine, file, MA_SOUND_FLAG_STREAM,
                                NULL, NULL, &impl->music) != MA_SUCCESS)
    {
        std::cout << "Could not load " << file << std::endl;
        return;
    }

    impl->musicLoaded = true;
    impl->currentMusic = music;
    ma_sound_set_looping(&impl->music, loop ? MA_TRUE : MA_FALSE);
    ma_sound_set_volume(&impl->music, MUSIC_VOLUME);
    ma_sound_start(&impl->music);
}

void Audio::stopMusic()
{
    if (impl->musicLoaded)
    {
        ma_sound_uninit(&impl->music);
        impl->musicLoaded = false;
    }
    impl->currentMusic = Music::NONE;
}
