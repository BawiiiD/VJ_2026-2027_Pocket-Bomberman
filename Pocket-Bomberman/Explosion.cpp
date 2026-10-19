#include "Explosion.h"

Explosion::Explosion(glm::ivec2 tile,
                     int tileSize,
                     glm::ivec2 offset,
                     ShaderProgram &program)
    : tile(tile),
      tileSize(tileSize),
      remainingMs(500),
      visual("images/gameplay/explosion.png", tile, tileSize, offset, program)
{
}

void Explosion::update(int dt)
{
    remainingMs -= dt;
}

bool Explosion::finished() const
{
    return remainingMs <= 0;
}

glm::ivec2 Explosion::getTilePos() const
{
    return tile;
}

glm::vec4 Explosion::bounds() const
{
    return glm::vec4(
        tile.x * tileSize,
        tile.y * tileSize,
        tileSize,
        tileSize
    );
}

void Explosion::render() const
{
    visual.render();
}
