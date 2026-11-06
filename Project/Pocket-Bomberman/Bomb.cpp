#include "Bomb.h"
#include "Gameplay.h"
#include "Audio.h"

namespace
{
    constexpr int FUSE_MS = 2500;

    constexpr int BOMB_ROW = 2;
    constexpr int BOMB_FRAMES = 3;
    constexpr int BOMB_FPS = 6;
    // The bomb blinks faster just before exploding
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

    if (remainingMs <= HURRY_MS && sprite->animation() != HURRY)
    {
        sprite->changeAnimation(HURRY);
        tickMs = 0;
    }

    // One tick each time the fuse animation loops, so it speeds up with it
    tickMs -= deltaTime;
    if (tickMs <= 0 && remainingMs > 0)
    {
        Audio::instance().playSound(Sound::BOMB_TICK);
        const int fps = sprite->animation() == HURRY ? HURRY_FPS : BOMB_FPS;
        tickMs += 1000 * BOMB_FRAMES / fps;
    }

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
