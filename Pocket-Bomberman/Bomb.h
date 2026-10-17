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
    glm::vec4 bounds() const;

    // A bomb is solid when the player and enemies are not inside it
    bool isSolid() const;
    void setSolid(bool solid);

private:
    glm::ivec2 tile;
    int range;
    int remainingMs;
    int tileSize;
    bool triggered;
    bool solid;
    std::unique_ptr<Sprite> sprite;
};

#endif
