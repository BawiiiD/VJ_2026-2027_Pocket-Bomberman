#ifndef BOMB_H
#define BOMB_H

#include <memory>

#include "Sprite.h"

// A bomb sitting on a tile. It doesn't fall (bombs float in Pocket Bomberman,
// that's how you build stairs with them), it counts down its fuse while it
// blinks and ticks, and then tells BombSystem it's ready to explode.
// The explosion itself (which tiles burn) is BombSystem's job.
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

    // Makes the bomb explode right away, without waiting for the fuse.
    // Used when the fire of another bomb (or a ray) reaches it
    void trigger();
    // True when it has to explode: the fuse ran out or it was triggered
    bool ready() const;

    glm::ivec2 getTilePos() const;
    int getRange() const;

private:
    glm::ivec2 tile;
    // How many tiles the fire reaches in each direction (the Fire Up level when it was placed)
    int range;
    // Time left on the fuse
    int remainingMs;
    // Time until the next tick sound
    int tickMs;
    bool triggered;
    std::unique_ptr<Sprite> sprite;
};

#endif
