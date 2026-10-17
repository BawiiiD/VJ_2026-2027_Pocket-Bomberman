#ifndef EXPLOSION_H
#define EXPLOSION_H

#include <memory>
#include <vector>

#include "Sprite.h"

class Explosion
{
public:
    // The order matches the columns of images/explosion.png
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
    };

    Explosion(const std::vector<Cell> &cells,
              int tileSize,
              Texture &texture,
              ShaderProgram &program);

    void update(int deltaTime);
    void render();

    bool finished() const;
    // True if any tile of the explosion overlaps the box
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
