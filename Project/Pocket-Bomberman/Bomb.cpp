#include "Bomb.h"
#include "Gameplay.h"
#include "Audio.h"

namespace
{
    // A bomb explodes 2.5 s after being placed
    constexpr int FUSE_MS = 2500;

    // Its 3 frames are the first cells of row 2 of the atlas
    constexpr int BOMB_ROW = 2;
    constexpr int BOMB_FRAMES = 3;
    constexpr int BOMB_FPS = 6;
    // In the last 0.8 s the bomb blinks much faster, as a warning to run away
    constexpr int HURRY_MS = 800;
    constexpr int HURRY_FPS = 14;

    enum BombAnimation
    {
        FUSE,
        HURRY
    };
}

Bomb::Bomb(glm::ivec2 tile,
           int range,
           int tileSize,
           Texture &atlas,
           ShaderProgram &program)
    : tile(tile),
      range(range),
      remainingMs(FUSE_MS),
      tickMs(1000 * BOMB_FRAMES / BOMB_FPS),
      triggered(false)
{
    sprite.reset(Sprite::createSprite(
        glm::vec2(float(tileSize)),
        atlasCellSize(),
        &atlas,
        &program
    ));

    // Both animations use the same frames, only the speed changes
    sprite->setNumberAnimations(2);
    sprite->setAnimationSpeed(FUSE, BOMB_FPS);
    sprite->setAnimationSpeed(HURRY, HURRY_FPS);
    for (int frame = 0; frame < BOMB_FRAMES; ++frame)
    {
        sprite->addKeyframe(FUSE, atlasCell(frame, BOMB_ROW));
        sprite->addKeyframe(HURRY, atlasCell(frame, BOMB_ROW));
    }
    sprite->changeAnimation(FUSE);
    sprite->setPosition(glm::vec2(tile * tileSize));
}

void Bomb::update(int deltaTime)
{
    sprite->update(deltaTime);

    remainingMs -= deltaTime;

    // Switch to the fast blinking once, the first time we enter the last 0.8 s.
    // The tick also sounds right away to mark the change
    if (remainingMs <= HURRY_MS && sprite->animation() != HURRY)
    {
        sprite->changeAnimation(HURRY);
        tickMs = 0;
    }

    // One tick each time the fuse animation loops (3 frames), so the sound goes
    // faster when the blinking does, without having a separate timer for it
    tickMs -= deltaTime;
    if (tickMs <= 0 && remainingMs > 0)
    {
        Audio::instance().playSound(Sound::BOMB_TICK);
        const int fps = sprite->animation() == HURRY ? HURRY_FPS : BOMB_FPS;
        tickMs += 1000 * BOMB_FRAMES / fps;
    }

    // We only mark it; BombSystem is the one that removes it and makes the fire
    if (remainingMs <= 0)
        triggered = true;
}

void Bomb::render()
{
    sprite->render();
}

void Bomb::trigger()
{
    triggered = true;
}

bool Bomb::ready() const
{
    return triggered;
}

glm::ivec2 Bomb::getTilePos() const
{
    return tile;
}

int Bomb::getRange() const
{
    return range;
}
