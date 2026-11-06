#ifndef GAMEPLAY_H
#define GAMEPLAY_H

#include <glm/glm.hpp>

class TileMap;

// Small helpers shared by all the entities of the game: finding things in the
// sprite atlas, checking if two boxes touch, and moving a box against the walls.

// images/sprites.png is 512x512 and it's split in 16x16 cells
constexpr float ATLAS_SIZE = 512.f;
constexpr int ATLAS_CELL = 16;

// Texture coordinates of a pixel of the atlas (OpenGL wants 0..1, not pixels)
glm::vec2 atlasPixel(int x, int y);
// Texture coordinates of the top-left corner of a cell, so we can write
// atlasCell(2, 5) instead of doing the math every time
glm::vec2 atlasCell(int column, int row);
// How big one cell is in texture coordinates
glm::vec2 atlasCellSize();

// Boxes are (x, y, width, height) in pixels.
// True if the two boxes overlap. Just touching at the border doesn't count
bool overlaps(const glm::vec4 &first, const glm::vec4 &second);
// The same box, "amount" pixels smaller on every side. We use it so that
// a hit needs a bit of real overlap, which feels fairer
glm::vec4 shrink(const glm::vec4 &box, float amount);

// Moves a box along one axis (0 = x, 1 = y) and stops it against the walls.
// Returns true if something blocked it (we use it to know if we landed,
// hit a ceiling or bumped into a wall)
bool moveAndCollide(const TileMap &map,
                    glm::vec2 &pos,
                    const glm::vec2 &size,
                    float delta,
                    int axis);

#endif
