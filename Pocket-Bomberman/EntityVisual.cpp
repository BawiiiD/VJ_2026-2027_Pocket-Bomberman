#include <stdexcept>
#include "EntityVisual.h"

EntityVisual::EntityVisual(const std::string &image,
                           glm::ivec2 tile,
                           int tileSize,
                           glm::ivec2 offset,
                           ShaderProgram &program)
{
    if (!texture.loadFromFile(image, TEXTURE_PIXEL_FORMAT_RGBA))
    {
        throw std::runtime_error("Cannot load " + image);
    }

    texture.setMinFilter(GL_NEAREST);
    texture.setMagFilter(GL_NEAREST);

    sprite.reset(Sprite::createSprite(
        glm::vec2(tileSize),
        glm::vec2(1.f),
        &texture,
        &program
    ));

    sprite->setNumberAnimations(1);
    sprite->setAnimationSpeed(0, 1);
    sprite->addKeyframe(0, glm::vec2(0.f));
    sprite->changeAnimation(0);

    sprite->setPosition(
        glm::vec2(offset) + glm::vec2(tile * tileSize)
    );
}

void EntityVisual::render() const
{
    sprite->render();
}
