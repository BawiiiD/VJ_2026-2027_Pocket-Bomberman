#include <cmath>

#include "Gameplay.h"
#include "TileMap.h"

glm::vec2 atlasPixel(int x, int y)
{
    return glm::vec2(x / ATLAS_SIZE, y / ATLAS_SIZE);
}

glm::vec2 atlasCell(int column, int row)
{
    return atlasPixel(column * ATLAS_CELL, row * ATLAS_CELL);
}

glm::vec2 atlasCellSize()
{
    return glm::vec2(ATLAS_CELL / ATLAS_SIZE);
}

bool overlaps(const glm::vec4 &first, const glm::vec4 &second)
{
    // Two boxes overlap when they overlap on x AND on y. With < instead of <=
    // two boxes that only share an edge don't count as touching
    const bool overlapX =
        first.x < second.x + second.z &&
        first.x + first.z > second.x;

    const bool overlapY =
        first.y < second.y + second.w &&
        first.y + first.w > second.y;

    return overlapX && overlapY;
}

glm::vec4 shrink(const glm::vec4 &box, float amount)
{
    return glm::vec4(
        box.x + amount,
        box.y + amount,
        box.z - 2.f * amount,
        box.w - 2.f * amount
    );
}

// Moving one axis at a time makes collisions easy: if we're blocked we know
// exactly which side we hit, and we can still slide along the other axis
// (that's why you don't get stuck when you jump against a wall).
bool moveAndCollide(const TileMap &map,
                    glm::vec2 &pos,
                    const glm::vec2 &size,
                    float delta,
                    int axis)
{
    if (delta == 0.f)
        return false;

    // We need where we were, so a character can walk out of a bomb it's standing in
    const glm::vec2 previousPos = pos;
    pos[axis] += delta;

    if (!map.collides(pos, size, previousPos))
        return false;

    // We went into a solid tile: put the box right against its edge.
    // This only works because nothing moves more than one tile in a frame
    const float tileSize = float(map.getTileSize());

    if (delta > 0.f)
        pos[axis] = std::floor((pos[axis] + size[axis]) / tileSize) * tileSize - size[axis];
    else
        pos[axis] = (std::floor(pos[axis] / tileSize) + 1.f) * tileSize;

    return true;
}
