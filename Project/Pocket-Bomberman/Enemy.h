#ifndef ENEMY_H
#define ENEMY_H

#include <memory>
#include <vector>
#include <glm/glm.hpp>

#include "Sprite.h"
#include "TileMap.h"

// Base class of the small enemies. The three of them fall, collide with the
// map, animate and die the same way, so all of that is written once here.
// The only thing that changes is how they decide to move, and that's the
// think() that each subclass writes. Scene keeps them all in one list of
// Enemy pointers and never needs to know which kind each one is.
class Enemy
{
public:
    enum class Type
    {
        WALKER,     // Patrols a platform, turning around at walls and edges
        HOPPER,     // Waits and then jumps towards the player
        CHASER      // Patrols slowly, runs after the player when it sees him
    };

    // Makes the right subclass from the type read in the level file
    static std::unique_ptr<Enemy> create(Type type);

    // Virtual, because Scene deletes them through an Enemy pointer
    virtual ~Enemy() = default;

    void init(Texture &atlas,
              ShaderProgram &program,
              TileMap *tileMap,
              const glm::ivec2 &tile);

    void update(int deltaTime, const glm::vec2 &playerCenter);
    void render();

    // Hit by the fire (or the K cheat)
    void kill();
    // Dying enemies can no longer hurt the player
    bool isDying() const;
    // The death animation has finished, it can be removed
    bool isDead() const;
    glm::vec4 getBounds() const;

protected:
    // Each enemy type has its own row of the atlas and its own frame columns
    Enemy(int atlasRow,
          std::vector<int> moveColumns,
          std::vector<int> deathColumns);

    // Each enemy type decides here where to go (it sets velocity.x, and can jump
    // by setting velocity.y). It's called before moving, so onGround and hitWall
    // are what happened last frame
    virtual void think(float dt, const glm::vec2 &playerCenter) = 0;

    // Is there floor in front of the enemy in the given direction?
    bool groundAhead(int dir) const;

protected:
    // Protected so the subclasses can read and change them in think()
    TileMap *map;
    glm::vec2 position;   // top-left of the hitbox, like the player
    glm::vec2 velocity;   // pixels per second
    int direction;        // -1 left, +1 right
    bool onGround;
    bool hitWall;         // it bumped into a wall last frame

private:
    enum State
    {
        ALIVE,
        DYING,
        DEAD
    };

    // Every enemy has its own row in the atlas and its own frames
    int atlasRow;
    std::vector<int> moveColumns;
    std::vector<int> deathColumns;
    State state;
    // A dying enemy stays on screen for a moment before disappearing
    int dyingMs;
    std::unique_ptr<Sprite> sprite;
};

class Walker : public Enemy
{
public:
    Walker();

protected:
    void think(float dt, const glm::vec2 &playerCenter) override;
};

class Hopper : public Enemy
{
public:
    Hopper();

protected:
    void think(float dt, const glm::vec2 &playerCenter) override;

private:
    // Time until the next jump
    float waitMs;
};

class Chaser : public Enemy
{
public:
    Chaser();

protected:
    void think(float dt, const glm::vec2 &playerCenter) override;
};

#endif // ENEMY_H
