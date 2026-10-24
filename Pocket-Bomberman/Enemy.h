#ifndef ENEMY_H
#define ENEMY_H

#include <memory>
#include <vector>
#include <glm/glm.hpp>

#include "Sprite.h"
#include "TileMap.h"

// Base class of the small enemies. Each subclass decides how it moves in think();
// the base class takes care of gravity, collisions, animations and dying.
class Enemy
{
public:
    enum class Type
    {
        WALKER,     // Patrols a platform, turning around at walls and edges
        HOPPER,     // Waits and then jumps towards the player
        CHASER      // Patrols slowly, runs after the player when it sees him
    };

    static std::unique_ptr<Enemy> create(Type type);

    virtual ~Enemy() = default;

    void init(Texture &atlas,
              ShaderProgram &program,
              TileMap *tileMap,
              const glm::ivec2 &tile);

    void update(int deltaTime, const glm::vec2 &playerCenter);
    void render();

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

    virtual void think(float dt, const glm::vec2 &playerCenter) = 0;

    // Is there floor in front of the enemy in the given direction?
    bool groundAhead(int dir) const;

protected:
    TileMap *map;
    glm::vec2 position;
    glm::vec2 velocity;
    int direction;
    bool onGround;
    bool hitWall;

private:
    enum State
    {
        ALIVE,
        DYING,
        DEAD
    };

    int atlasRow;
    std::vector<int> moveColumns;
    std::vector<int> deathColumns;
    State state;
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
