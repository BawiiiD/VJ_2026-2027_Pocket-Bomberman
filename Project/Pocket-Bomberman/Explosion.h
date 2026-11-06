#ifndef EXPLOSION_H
#define EXPLOSION_H

#include <memory>
#include <vector>

#include "Sprite.h"

// The fire of one bomb: a cross made of tiles that lasts 0.6 s.
// BombSystem decides which tiles burn and which piece of fire goes in each
// one (center, middle of an arm, or the tip). Explosion only draws them,
// growing and then shrinking, and tells if something is touching the fire.
class Explosion
{
public:
    // The order matches the columns of images/explosion.png, so the piece
    // number is also its column in the image
    enum class Piece
    {
        CENTER,
        HORIZONTAL,
        VERTICAL,
        END_LEFT,
        END_RIGHT,
        END_UP,
        END_DOWN
    };

    struct Cell
    {
        glm::ivec2 tile;
        Piece piece;
        // A bush that just broke shows a flame tip, but that flame doesn't hurt
        bool damaging = true;
    };

    Explosion(const std::vector<Cell> &cells,
              int tileSize,
              Texture &texture,
              ShaderProgram &program);

    void update(int deltaTime);
    void render();

    // After 0.6 s the explosion is over and BombSystem deletes it
    bool finished() const;
    // True if any damaging tile of the explosion overlaps the box
    bool hits(const glm::vec4 &box) const;
    const std::vector<Cell> &getCells() const;

private:
    glm::vec2 pieceFrame(Piece piece, int phase) const;

private:
    std::vector<Cell> cells;
    int tileSize;
    int elapsedMs;
    std::unique_ptr<Sprite> sprite;
};

#endif
