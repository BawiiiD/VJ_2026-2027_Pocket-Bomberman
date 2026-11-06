#ifndef _AUDIO_INCLUDE
#define _AUDIO_INCLUDE

#include <memory>

// Short sound effects (sounds/*.wav). The order has to match the SOUNDS
// table in Audio.cpp, because we use the enum value as the index
enum class Sound
{
    BOMB_PLACE,
    BOMB_TICK,
    EXPLOSION,
    BUSH_BREAK,
    ITEM_GET,
    JUMP,
    LAND,
    PLAYER_DEATH,
    HEART_LOST,
    ENEMY_DEATH,
    BOSS_HIT,
    BOSS_ATTACK,
    DOOR_OPEN,
    TIME_WARNING,
    MENU_MOVE,
    MENU_SELECT,

    // Count of sounds
    COUNT
};

// Music and jingles (sounds/*.mp3). Only one plays at a time, and the order
// matches MUSIC_FILES in Audio.cpp
enum class Music
{
    NONE,
    TITLE,
    MAP,
    STAGE1,
    STAGE2,
    STAGE3,
    STAGE4,
    BOSS,
    CLEAR,
    TIME_UP,
    GAME_OVER,
    VICTORY
};

// Audio plays the sound effects and the music, using miniaudio. It's a
// singleton like Game, so anyone can just call Audio::instance().playSound(...)
// without having to pass the audio object around.
// If there is no audio device or a file is missing, that sound is simply not
// played.
class Audio
{
private:
    Audio();

public:
    static Audio &instance()
    {
        static Audio audio;
        return audio;
    }

    ~Audio();

    void init();
    void shutdown();

    // Fire and forget: the effect plays on its own until it ends
    void playSound(Sound sound);

    // Replaces the current music. Does nothing if this music is already playing,
    // so we can call it every time a level loads without restarting the song.
    // Jingles are played with loop = false
    void playMusic(Music music, bool loop = true);
    void stopMusic();

private:
    struct Impl;
    std::unique_ptr<Impl> impl;
};

#endif // _AUDIO_INCLUDE
