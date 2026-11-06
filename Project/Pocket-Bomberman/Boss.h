#ifndef BOSS_H
#define BOSS_H

#include <memory>
#include <vector>
#include <glm/glm.hpp>

#include "Sprite.h"
#include "TileMap.h"

// Something the boss throws. Seeds fly in an arc (they have gravity) and birds
// fly across in a gentle wave. Both go through walls and platforms, so you
// can't just hide under a platform; you have to dodge them, or blow them up.
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

    // The map is only used to know when it has left the arena
    void update(int deltaTime, const TileMap &map);
    void render();

    // When it hits the player or the fire reaches it
    void destroy();
    bool isAlive() const;
    glm::vec4 getBounds() const;

private:
    Kind kind;
    // Top-left of its 10x10 hitbox (the sprite is 16x16 around it)
    glm::vec2 position;
    glm::vec2 velocity;
    // Time left before the projectile disappears, just in case it never leaves the map
    int lifeMs;
    // Projectiles expire, leave the arena, or are destroyed by combat collisions.
    bool alive;
    std::unique_ptr<Sprite> sprite;
};

// Final boss: the big tree of level 5. It isn't an Enemy because it works
// differently: it's 32x32, it has health, it walks from side to side without
// falling, and it creates projectiles.
// It attacks every few seconds (seeds, seeds, bird, and again), and when it
// has 2 health or less it moves and attacks faster.
class Boss
{
public:
    Boss();

    void init(Texture &atlas,
              ShaderProgram &program,
              TileMap *tileMap,
              const glm::ivec2 &tile);

    // The projectiles list belongs to Scene: the boss only adds new ones to it,
    // and Scene moves, draws and deletes them (they keep flying if the boss dies)
    void update(int deltaTime,
                const glm::vec2 &playerCenter,
                std::vector<std::unique_ptr<Projectile>> &projectiles);
    void render();

    // An explosion touched it. Returns false if it didn't count (still invulnerable)
    bool hit();
    // Straight to the death animation (last hit, or the K cheat)
    void kill();

    // True as soon as it starts dying
    bool isDying() const;
    // True when the death animation is over: then the door appears
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

    // Kept so it can create projectiles whenever it attacks
    Texture *atlas;
    ShaderProgram *program;
    TileMap *map;
    std::unique_ptr<Sprite> sprite;

    // Top-left of the sprite (unlike the player, here it's not the hitbox)
    glm::vec2 position;
    State state;
    int moveDirection;    // -1 left, +1 right
    int health;
    int attackTimerMs;    // time until the next attack
    int attackCount;      // used to choose seeds or bird
    int poseMs;           // time left in the attack pose (it doesn't walk meanwhile)
    int invulnerableMs;   // time left after a hit
    int dyingMs;          // time left shaking before it disappears
};

#endif
