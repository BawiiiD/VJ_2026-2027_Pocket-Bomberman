#ifndef ENTITY_VISUAL_H
#define ENTITY_VISUAL_H

#include <memory>
#include <string>
#include "Sprite.h"

class EntityVisual
{
public:
    EntityVisual(const std::string &image,
                 glm::ivec2 tile,
                 int tileSize,
                 glm::ivec2 offset,
                 ShaderProgram &program);

    void render() const;

private:
    Texture texture;
    std::unique_ptr<Sprite> sprite;
};

#endif