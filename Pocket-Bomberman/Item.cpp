#include "Item.h"

Item::Item(glm::ivec2 tile,
           ItemType type,
           int tileSize,
           glm::ivec2 offset,
           ShaderProgram &program)
    : tile(tile),
      type(type),
      tileSize(tileSize),
      visual("images/gameplay/item.png", tile, tileSize, offset, program)
{
}

ItemType Item::getType() const
{
    return type;
}

glm::ivec2 Item::getTilePos() const
{
    return tile;
}

glm::vec4 Item::bounds() const
{
    return glm::vec4(
        tile.x * tileSize,
        tile.y * tileSize,
        tileSize,
        tileSize
    );
}

void Item::render() const
{
    visual.render();
}
