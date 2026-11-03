#ifndef BOMB_H
#define BOMB_H

#include <memory>

#include "Sprite.h"

class Bomb
{
public:
    Bomb(glm::ivec2 tile,
         int range,
         int tileSize,
         Texture &atlas,
         ShaderProgram &program);

    void update(int deltaTime);
    void render();

    // Makes the bomb explode on the next update (chain reactions)
    void trigger();
    bool ready() const;

    glm::ivec2 getTilePos() const;
    int getRange() const;

private:
    glm::ivec2 tile;
    int range;
    int remainingMs;
    // Time until the next tick sound
    int tickMs;
    bool triggered;
    std::unique_ptr<Sprite> sprite;
};

#endif
