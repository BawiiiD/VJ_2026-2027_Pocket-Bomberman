#ifndef _AUDIO_INCLUDE
#define _AUDIO_INCLUDE

#include <memory>

// Short sound effects (sounds/*.wav)
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

    COUNT
};

// Music and jingles (sounds/*.mp3). Only one plays at a time
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

// Audio is a singleton that plays the sound effects and the music with miniaudio.
// If there is no audio device or a file is missing, that sound is simply not played.
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

    void playSound(Sound sound);

    // Does nothing if this music is already playing
    void playMusic(Music music, bool loop = true);
    void stopMusic();

private:
    // The miniaudio objects are hidden in Audio.cpp, so that miniaudio.h
    // (a very big header) is only compiled once
    struct Impl;
    std::unique_ptr<Impl> impl;
};

#endif // _AUDIO_INCLUDE
