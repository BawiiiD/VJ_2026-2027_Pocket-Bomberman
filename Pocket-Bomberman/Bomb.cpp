#include "Bomb.h"

Bomb::Bomb(glm::ivec2 tile,
           int range,
           int fuseMs,
           bool remote,
           int tileSize,
           glm::ivec2 offset,
           ShaderProgram &program)
    : tile(tile),
      range(range),
      remainingMs(fuseMs),
      tileSize(tileSize),
      remote(remote),
      triggered(false),
      visual("images/gameplay/bomb.png", tile, tileSize, offset, program)
{
}

void Bomb::update(int dt)
{
    if (!remote)
    {
        remainingMs -= dt;

        if (remainingMs <= 0)
        {
            triggered = true;
        }
    }
}

void Bomb::trigger()
{
    triggered = true;
}

bool Bomb::ready() const
{
    return triggered;
}

bool Bomb::isRemote() const
{
    return remote;
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

void Bomb::render() const
{
    visual.render();
}
