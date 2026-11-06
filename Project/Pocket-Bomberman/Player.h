#ifndef _PLAYER_INCLUDE
#define _PLAYER_INCLUDE

#include <memory>
#include <glm/glm.hpp>

#include "Sprite.h"
#include "TileMap.h"
#include "ItemType.h"

// Abilities improved by the items. They are kept between levels.
// They're in their own struct so Scene can save them in one go when a level
// starts, and give them back if Bomberman dies (he keeps the items he had at
// the start of the level, and loses the ones he picked up in the failed try).
struct PlayerStats
{
    int maxBombs = 1;      // Bombs on the map at the same time (Bomb Up)
    int fireRange = 1;     // Tiles the fire reaches in each direction (Fire Up)
    int speedLevel = 0;    // 0 to 3 (Speed Up)
    bool heart = false;    // Survives one hit (Heart)
};

// Bomberman: reads the keys, moves with gravity and collisions, picks his
// animation, and knows if he's alive, dying or celebrating at the door.
// The rules about what hurts him are in CollisionSystem; Player only decides
// what a hit does to him.
class Player
{
public:
    Player();

    void init(Texture &atlas, ShaderProgram &shaderProgram);

    void update(int deltaTime);
    void render();

    // Call init and setTileMap before placing or updating the player.
    void setTileMap(TileMap *tileMap);
    // Places the player standing in the given tile (start of the level)
    void setTile(const glm::ivec2 &tile);
    // The camera, the enemies and the door use the center of the hitbox
    glm::vec2 getCenter() const;
    // The hitbox as (x, y, width, height), for every collision
    glm::vec4 getBounds() const;
    // The tile where his center is
    glm::ivec2 getTile() const;

    // False once the death animation has finished (Scene waits for it before
    // restarting the level, so you see the whole animation)
    bool isAlive() const;
    // True as soon as he's hit, while the death animation plays
    bool isDying() const;

    // Returns true if the hit kills the player. The heart, god mode and the
    // blinking time after losing the heart protect him.
    bool hurt(bool byBomb);
    // Kills the player even if he has the heart (time out)
    void kill();
    // Victory pose when reaching the exit door
    void celebrate();

    void applyItem(ItemType type);
    const PlayerStats &getStats() const;
    void setStats(const PlayerStats &newStats);

    void setGodMode(bool enabled);
    bool isGodMode() const;

private:
    enum PlayerState
    {
        NORMAL,       // Playing
        DYING,        // Death animation playing
        DEAD,         // Animation over
        CELEBRATING   // Reached the door: no more control
    };

    void updateAnimation(bool walking);
    void updateSpritePosition();

private:
    std::unique_ptr<Sprite> sprite;
    TileMap *map;

    // Position of the hitbox (top-left corner) in pixels. It's a float because he
    // moves less than a pixel per frame; we only round it when drawing
    glm::vec2 posPlayer;

    // Vertical speed in pixels per second (negative = going up). There's no
    // horizontal speed: walking moves him directly
    float velocityY;
    // Time left to still jump after walking off a ledge
    int coyoteMs;
    // Time left blinking after losing the heart
    int invulnerableMs;

    bool onGround;
    // Was the jump key down last frame? Used to jump only on a new press
    bool jumpWasPressed;
    bool facingRight;
    bool godMode;

    PlayerState state;
    PlayerStats stats;
};

#endif // _PLAYER_INCLUDE
