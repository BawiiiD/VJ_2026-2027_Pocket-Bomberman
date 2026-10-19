#ifndef ITEM_H
#define ITEM_H

#include "ItemType.h"
#include "EntityVisual.h"

class Item
{
public:
    Item(glm::ivec2 tile,
         ItemType type,
         int tileSize,
         glm::ivec2 offset,
         ShaderProgram &program);

    ItemType getType() const;
    glm::ivec2 getTilePos() const;
    glm::vec4 bounds() const;
    void render() const;

private:
    glm::ivec2 tile;
    ItemType type;
    int tileSize;
    EntityVisual visual;
};

#endif // ITEM_H
