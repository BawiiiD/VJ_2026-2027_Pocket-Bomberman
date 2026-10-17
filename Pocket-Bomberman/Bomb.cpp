#include "Bomb.h"
#include "Gameplay.h"

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
      tileSize(tileSize),
      triggered(false),
      solid(false)
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
        sprite->changeAnimation(HURRY);

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

glm::vec4 Bomb::bounds() const
{
    return glm::vec4(
        tile.x * tileSize,
        tile.y * tileSize,
        tileSize,
        tileSize
    );
}

bool Bomb::isSolid() const
{
    return solid;
}

void Bomb::setSolid(bool value)
{
    solid = value;
}
