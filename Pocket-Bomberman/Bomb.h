#ifndef BOMB_H
#define BOMB_H

#include "EntityVisual.h"

class Bomb
{
public:
    Bomb(glm::ivec2 tile,
         int range,
         int fuseMs,
         bool remote,
         int tileSize,
         glm::ivec2 offset,
         ShaderProgram &program);

    void update(int dt);
    void trigger();
    bool ready() const;
    bool isRemote() const;
    glm::ivec2 getTilePos() const;
    int getRange() const;
    glm::vec4 bounds() const;
    void render() const;

private:
    glm::ivec2 tile;
    int range;
    int remainingMs;
    int tileSize;
    bool remote;
    bool triggered;
    EntityVisual visual;
};

#endif
