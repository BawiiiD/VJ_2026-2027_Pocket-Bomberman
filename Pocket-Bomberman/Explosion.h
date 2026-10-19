#ifndef EXPLOSION_H
#define EXPLOSION_H

#include "EntityVisual.h"

class Explosion
{
public:
    Explosion(glm::ivec2 tile,
              int tileSize,
              glm::ivec2 offset,
              ShaderProgram &program);

    void update(int dt);
    bool finished() const;
    glm::ivec2 getTilePos() const;
    glm::vec4 bounds() const;
    void render() const;

private:
    glm::ivec2 tile;
    int tileSize;
    int remainingMs;
    EntityVisual visual;
};

#endif
