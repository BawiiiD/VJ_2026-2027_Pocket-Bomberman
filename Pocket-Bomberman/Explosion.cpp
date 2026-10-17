#include <algorithm>

#include "Explosion.h"
#include "Gameplay.h"

namespace
{
    constexpr int DURATION_MS = 600;

    constexpr int CELL_SIZE = 16;
    constexpr int TEXTURE_WIDTH = 128;
    constexpr int TEXTURE_HEIGHT = 64;

    const int PHASE_SEQUENCE[] = { 0, 1, 2, 2, 2, 1, 0 };
    constexpr int PHASE_COUNT = sizeof(PHASE_SEQUENCE) / sizeof(PHASE_SEQUENCE[0]);
}

Explosion::Explosion(const std::vector<Cell> &cells,
                     int tileSize,
                     Texture &texture,
                     ShaderProgram &program)
    : cells(cells),
      tileSize(tileSize),
      elapsedMs(0)
{
    sprite.reset(Sprite::createSprite(
        glm::vec2(float(tileSize)),
        glm::vec2(float(CELL_SIZE) / TEXTURE_WIDTH, float(CELL_SIZE) / TEXTURE_HEIGHT),
        &texture,
        &program
    ));
}

void Explosion::update(int deltaTime)
{
    elapsedMs += deltaTime;
}

bool Explosion::finished() const
{
    return elapsedMs >= DURATION_MS;
}

// Returns the texture coordinates of the given piece of fire at the given phase
glm::vec2 Explosion::pieceFrame(Piece piece, int phase) const
{
    return glm::vec2(
        float(static_cast<int>(piece) * CELL_SIZE) / TEXTURE_WIDTH,
        float(phase * CELL_SIZE) / TEXTURE_HEIGHT
    );
}

void Explosion::render()
{
    // The fire is animated in 7 phases, each lasting DURATION_MS / PHASE_COUNT milliseconds
    const int step = std::min(PHASE_COUNT - 1, elapsedMs * PHASE_COUNT / DURATION_MS);
    const int phase = PHASE_SEQUENCE[step];

    for (const Cell &cell : cells)
    {
        sprite->setFrame(pieceFrame(cell.piece, phase));
        sprite->setPosition(glm::vec2(cell.tile * tileSize));
        sprite->render();
    }
}

bool Explosion::hits(const glm::vec4 &box) const
{
    if (finished())
        return false;

    for (const Cell &cell : cells)
    {
        const glm::vec4 cellBox(
            cell.tile.x * tileSize,
            cell.tile.y * tileSize,
            tileSize,
            tileSize
        );

        // A little margin so that nearly touching the fire does not kill
        if (overlaps(shrink(cellBox, 2.f), box))
            return true;
    }

    return false;
}

const std::vector<Explosion::Cell> &Explosion::getCells() const
{
    return cells;
}
