#ifndef ITEM_H
#define ITEM_H

#include <memory>

#include "ItemType.h"
#include "Sprite.h"

// A power-up lying where a bush was. It floats a little so it's easy to see,
// and Scene removes it when Bomberman touches it (CollisionSystem applies its effect).
class Item
{
public:
    Item(glm::ivec2 tile,
         ItemType type,
         int tileSize,
         Texture &atlas,
         ShaderProgram &program);

    void update(int deltaTime);
    void render();

    ItemType getType() const;
    glm::ivec2 getTilePos() const;
    // The whole tile, so you just have to walk into it to pick it up
    glm::vec4 bounds() const;

private:
    glm::ivec2 tile;
    ItemType type;
    int tileSize;
    int elapsedMs;
    // When the player collects the item, it is removed from the scene and the sprite is deleted
    std::unique_ptr<Sprite> sprite;
};

#endif // ITEM_H
