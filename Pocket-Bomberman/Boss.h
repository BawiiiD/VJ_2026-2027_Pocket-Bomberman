#ifndef BOSS_H
#define BOSS_H

#include <memory>
#include <vector>
#include <glm/glm.hpp>

#include "Sprite.h"
#include "TileMap.h"

// Boss attacks pass through terrain: seeds follow an arc and birds fly across.
class Projectile
{
public:
    // Projectile types
    enum class Kind
    {
        SEED,
        BIRD
    };

    Projectile(Kind kind,
               const glm::vec2 &position,
               const glm::vec2 &velocity,
               Texture &atlas,
               ShaderProgram &program);

    void update(int deltaTime, const TileMap &map);
    void render();

    void destroy();
    bool isAlive() const;
    glm::vec4 getBounds() const;

private:
    Kind kind;
    glm::vec2 position;
    glm::vec2 velocity;
    // Time left before the projectile disappears
    int lifeMs;
    // Projectiles expire, leave the arena, or are destroyed by combat collisions.
    bool alive;
    std::unique_ptr<Sprite> sprite;
};

// Final boss: the big tree
class Boss
{
public:
    Boss();

    void init(Texture &atlas,
              ShaderProgram &program,
              TileMap *tileMap,
              const glm::ivec2 &tile);

    void update(int deltaTime,
                const glm::vec2 &playerCenter,
                std::vector<std::unique_ptr<Projectile>> &projectiles);
    void render();

    bool hit();
    void kill();

    bool isDying() const;
    bool isDefeated() const;
    int getHealth() const;
    int getMaxHealth() const;
    glm::vec4 getBounds() const;

private:
    void attack(const glm::vec2 &playerCenter,
                std::vector<std::unique_ptr<Projectile>> &projectiles);

private:
    enum State
    {
        FIGHTING,
        DYING,
        DEFEATED
    };

    Texture *atlas;
    ShaderProgram *program;
    TileMap *map;
    std::unique_ptr<Sprite> sprite;

    glm::vec2 position;
    State state;
    int moveDirection;
    int health;
    int attackTimerMs;
    int attackCount;
    int poseMs;
    int invulnerableMs;
    int dyingMs;
};

#endif
