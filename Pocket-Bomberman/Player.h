#ifndef _PLAYER_INCLUDE
#define _PLAYER_INCLUDE

#include <glm/glm.hpp>

#include "Sprite.h"
#include "TileMap.h"

class Player
{
public:
    Player();
    ~Player();

    void init(const glm::ivec2 &tileMapPos,
              ShaderProgram &shaderProgram);

    void update(int deltaTime);
    void render();

    void setTileMap(TileMap *tileMap);
    void setPosition(const glm::vec2 &pos);

    bool isAlive() const;

    void hitByMonster();
    void hitByBomb();

private:
    enum PlayerState
    {
        NORMAL,
        DYING_BY_MONSTER,
        DYING_BY_BOMB,
        DEAD
    };

    void updateAnimation(bool walking);
    void updateSpritePosition();

private:
    Texture spritesheet;
    Sprite *sprite;
    TileMap *map;

    glm::ivec2 tileMapDispl;
    glm::vec2 posPlayer;

    float velocityY;
    float moveSpeed;

    bool onGround;
    bool jumpWasPressed;
    bool facingRight;

    bool alive;
    PlayerState state;
};

#endif // _PLAYER_INCLUDE