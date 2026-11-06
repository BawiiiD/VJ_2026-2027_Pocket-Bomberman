#include "Explosion.h"
#include "Gameplay.h"

namespace
{
    constexpr int DURATION_MS = 600;

    // images/explosion.png: 16x16 pieces, one column per Piece and one row per
    // size of the fire (row 0 is the biggest)
    constexpr int CELL_SIZE = 16;
    constexpr int TEXTURE_WIDTH = 128;
    constexpr int TEXTURE_HEIGHT = 64;

    // Which row is shown in each of the 7 steps of the explosion
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
    // One sprite for all the cells: render() moves it and changes its frame for each one
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
    // The fire is animated in 7 steps, each lasting DURATION_MS / PHASE_COUNT
    // milliseconds. A finished explosion is deleted before it's drawn, so step
    // never goes past the end of the table
    const int step = elapsedMs * PHASE_COUNT / DURATION_MS;
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
        if (!cell.damaging)
            continue;

        const glm::vec4 cellBox(
            cell.tile.x * tileSize,
            cell.tile.y * tileSize,
            tileSize,
            tileSize
        );

        // 2 pixels of margin, so just brushing the edge of the fire doesn't kill you
        if (overlaps(shrink(cellBox, 2.f), box))
            return true;
    }

    return false;
}

const std::vector<Explosion::Cell> &Explosion::getCells() const
{
    return cells;
}
