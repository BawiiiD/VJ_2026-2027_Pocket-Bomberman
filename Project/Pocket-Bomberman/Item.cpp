#include <cmath>

#include "Item.h"
#include "Gameplay.h"

namespace
{
    constexpr int ITEM_ROW = 4;
}

Item::Item(glm::ivec2 tile,
           ItemType type,
           int tileSize,
           Texture &atlas,
           ShaderProgram &program)
    : tile(tile),
      type(type),
      tileSize(tileSize),
      elapsedMs(0)
{
    sprite.reset(Sprite::createSprite(
        glm::vec2(float(tileSize)),
        atlasCellSize(),
        &atlas,
        &program
    ));
    sprite->setFrame(atlasCell(static_cast<int>(type), ITEM_ROW));
    sprite->setPosition(glm::vec2(tile * tileSize));
}

void Item::update(int deltaTime)
{
    elapsedMs += deltaTime;

    // Items float softly up and down so that they are easy to spot
    const float offset = std::round(std::sin(elapsedMs / 200.f) * 1.5f);
    sprite->setPosition(glm::vec2(tile * tileSize) + glm::vec2(0.f, offset));
}

void Item::render()
{
    sprite->render();
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
