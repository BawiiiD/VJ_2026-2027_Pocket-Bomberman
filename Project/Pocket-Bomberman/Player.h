#ifndef _PLAYER_INCLUDE
#define _PLAYER_INCLUDE

#include <memory>
#include <glm/glm.hpp>

#include "Sprite.h"
#include "TileMap.h"
#include "ItemType.h"

// Abilities improved by the items. They are kept between levels.
struct PlayerStats
{
    int maxBombs = 1;
    int fireRange = 1;
    int speedLevel = 0;
    bool heart = false;
};

class Player
{
public:
    Player();

    void init(Texture &atlas, ShaderProgram &shaderProgram);

    void update(int deltaTime);
    void render();

    // Call init and setTileMap before placing or updating the player.
    void setTileMap(TileMap *tileMap);
    // Places the player standing in the given tile
    void setTile(const glm::ivec2 &tile);
    glm::vec2 getCenter() const;
    glm::vec4 getBounds() const;
    glm::ivec2 getTile() const;

    // False once the death animation has finished
    bool isAlive() const;
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
        NORMAL,
        DYING,
        DEAD,
        CELEBRATING
    };

    void updateAnimation(bool walking);
    void updateSpritePosition();

private:
    std::unique_ptr<Sprite> sprite;
    TileMap *map;

    // Position of the hitbox (top-left corner) in pixels
    glm::vec2 posPlayer;

    float velocityY;
    int coyoteMs;
    int invulnerableMs;

    bool onGround;
    bool jumpWasPressed;
    bool facingRight;
    bool godMode;

    PlayerState state;
    PlayerStats stats;
};

#endif // _PLAYER_INCLUDE
