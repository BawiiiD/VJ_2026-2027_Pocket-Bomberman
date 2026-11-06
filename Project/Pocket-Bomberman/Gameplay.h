#ifndef GAMEPLAY_H
#define GAMEPLAY_H

#include <glm/glm.hpp>

class TileMap;

// Size in pixels of the sprite atlas
constexpr float ATLAS_SIZE = 512.f;
constexpr int ATLAS_CELL = 16;

// Returns the texture coordinates of a pixel in the atlas
glm::vec2 atlasPixel(int x, int y);
// Returns the texture coordinates of a cell in the atlas
glm::vec2 atlasCell(int column, int row);
// Returns the size of a cell in the atlas in texture coordinates
glm::vec2 atlasCellSize();

// Boxes are (x, y, width, height)
// Returns true if the two boxes overlap
bool overlaps(const glm::vec4 &first, const glm::vec4 &second);
// Returns a box shrunk by the given amount on all sides
glm::vec4 shrink(const glm::vec4 &box, float amount);

// Moves a box along one axis (0 = x, 1 = y) 
// Returns true if the movement was blocked.
bool moveAndCollide(const TileMap &map,
                    glm::vec2 &pos,
                    const glm::vec2 &size,
                    float delta,
                    int axis);

#endif
