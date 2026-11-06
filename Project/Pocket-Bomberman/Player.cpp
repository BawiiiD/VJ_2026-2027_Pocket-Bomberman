#include <algorithm>
#include <cmath>
#include <initializer_list>

#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include "Player.h"
#include "Game.h"
#include "Gameplay.h"
#include "Audio.h"

namespace
{
    // Size of the sprite on screen and of the hitbox used for collisions.
    // The hitbox is a bit smaller so that Bomberman fits through 1-tile gaps
    // without having to be perfectly aligned. It's 2 px in from the sides and
    // the top, and its bottom is the bottom of the sprite, so the feet touch the floor.
    constexpr float SPRITE_SIZE = 16.f;
    const glm::vec2 HITBOX_SIZE(12.f, 14.f);
    const glm::vec2 HITBOX_OFFSET(2.f, 2.f);

    // Physics constants for the player (pixels and seconds)
    constexpr float GRAVITY = 700.f;   // How fast the falling speed grows
    // A full jump rises about 55 px: enough to climb 3 tiles (48 px) but not 4
    constexpr float JUMP_SPEED = 285.f;
    // Falling never gets faster than this, so he never moves more than one
    // tile in a frame (moveAndCollide needs that)
    constexpr float MAX_FALL_SPEED = 280.f;
    constexpr float PLAYER_SPEED = 64.f;      // 4 tiles per second
    constexpr float SPEED_PER_LEVEL = 12.f;   // Extra speed for each Speed Up

    // Landing faster than this plays the landing sound (so just standing, or
    // stepping down a few pixels, is silent)
    constexpr float LAND_SOUND_SPEED = 100.f;

    // Jumping is still allowed a few ms after walking off a ledge
    constexpr int COYOTE_MS = 80;
    // Blinking time after the heart has been lost
    constexpr int INVULNERABLE_MS = 2000;

    // Maximum level of each item
    constexpr int MAX_BOMBS = 4;
    constexpr int MAX_FIRE = 4;
    constexpr int MAX_SPEED_LEVEL = 3;

    // All the player frames are in row 0 of images/sprites.png
    constexpr int PLAYER_ROW = 0;

    enum PlayerAnimation
    {
        STAND,
        MOVE,
        JUMP,
        FALL,
        DEATH_BY_BOMB,
        DEATH_BY_MONSTER,
        CELEBRATE,

        ANIMATION_COUNT
    };
}

Player::Player()
{
    map = nullptr;

    posPlayer = glm::vec2(0.f);

    velocityY = 0.f;
    coyoteMs = 0;
    invulnerableMs = 0;

    onGround = false;
    jumpWasPressed = false;
    facingRight = true;
    godMode = false;

    state = NORMAL;
}

void Player::init(Texture &atlas, ShaderProgram &shaderProgram)
{
    sprite.reset(Sprite::createSprite(
        glm::vec2(SPRITE_SIZE),
        atlasCellSize(),
        &atlas,
        &shaderProgram
    ));

    sprite->setNumberAnimations(ANIMATION_COUNT);

    auto addFrames = [&](int animation,
                         std::initializer_list<int> columns,
                         int framesPerSecond,
                         bool loop)
    {
        sprite->setAnimationSpeed(animation, framesPerSecond);
        sprite->setAnimationLoop(animation, loop);

        for (int column : columns)
            sprite->addKeyframe(animation, atlasCell(column, PLAYER_ROW));
    };

    addFrames(STAND, { 0 }, 1, true);
    addFrames(MOVE, { 0, 1, 0, 2 }, 8, true);
    // Air animations start with the crouched frame and then stay on the last one
    addFrames(JUMP, { 3, 4 }, 10, false);
    addFrames(FALL, { 3, 5 }, 10, false);
    // The death animations play only once: when they end, Player knows he's DEAD
    addFrames(DEATH_BY_BOMB, { 10, 11, 12 }, 6, false);
    addFrames(DEATH_BY_MONSTER, { 13, 14, 13, 14 }, 5, false);
    addFrames(CELEBRATE, { 6, 7 }, 4, true);

    sprite->changeAnimation(STAND);
    sprite->setFlipX(facingRight);

    updateSpritePosition();
}

void Player::update(int deltaTime)
{
    sprite->update(deltaTime);

    if (state == DEAD)
        return;

    // While dying he doesn't move, we just wait for the animation to end
    if (state == DYING)
    {
        if (sprite->animationFinished())
            state = DEAD;
        return;
    }

    if (invulnerableMs > 0)
        invulnerableMs -= deltaTime;

    const float dt = deltaTime / 1000.f;

    bool walking = false;
    const Game &game = Game::instance();

    // Horizontal movement (no control while celebrating)
    if (state == NORMAL)
    {
        const float speed = PLAYER_SPEED + SPEED_PER_LEVEL * stats.speedLevel;
        float moveX = 0.f;

        if (game.getKey(GLFW_KEY_LEFT))
        {
            facingRight = false;
            moveX = -speed * dt;
        }
        else if (game.getKey(GLFW_KEY_RIGHT))
        {
            facingRight = true;
            moveX = speed * dt;
        }

        // "walking" only if he really moved. Pushing against a wall shows him
        // standing instead of walking in place
        if (moveX != 0.f)
            walking = !moveAndCollide(*map, posPlayer, HITBOX_SIZE, moveX, 0);

        const bool jumpKey = game.getKey(GLFW_KEY_UP) || game.getKey(GLFW_KEY_Z);

        // A jump needs a NEW press (otherwise holding the key would make him
        // jump again as soon as he lands), and being on the floor or just
        // having left it (coyote time)
        if (jumpKey && !jumpWasPressed && (onGround || coyoteMs > 0))
        {
            velocityY = -JUMP_SPEED;
            onGround = false;
            coyoteMs = 0;   // used up, so no double jump
            Audio::instance().playSound(Sound::JUMP);
        }

        // Releasing the key early makes a shorter jump: we cut the speed going up.
        // Tap for a small hop, hold for the full jump
        if (!jumpKey && velocityY < -JUMP_SPEED * 0.4f)
            velocityY = -JUMP_SPEED * 0.4f;

        jumpWasPressed = jumpKey;
    }

    // Gravity and vertical movement. This also runs while celebrating, so if he
    // reached the door in the air he still falls to the floor
    velocityY = std::min(velocityY + GRAVITY * dt, MAX_FALL_SPEED);

    const bool wasOnGround = onGround;

    if (moveAndCollide(*map, posPlayer, HITBOX_SIZE, velocityY * dt, 1))
    {
        // Something blocked us, if we were going down it's the floor, if we were
        // going up it's a ceiling.
        onGround = velocityY > 0.f;
        if (onGround && !wasOnGround && velocityY > LAND_SOUND_SPEED)
            Audio::instance().playSound(Sound::LAND);
        velocityY = 0.f;
    }
    else
    {
        onGround = false;
    }

    // On the floor the coyote time is always full; once in the air it runs out
    if (onGround)
        coyoteMs = COYOTE_MS;
    else if (wasOnGround && velocityY >= 0.f)
        coyoteMs = COYOTE_MS;
    else
        coyoteMs = std::max(0, coyoteMs - deltaTime);

    // While celebrating we keep the victory animation
    if (state == NORMAL)
        updateAnimation(walking);

    updateSpritePosition();
}

void Player::render()
{
    // Blink while invulnerable
    if (invulnerableMs > 0 && (invulnerableMs / 80) % 2 == 0)
        return;

    sprite->render();
}

void Player::setTileMap(TileMap *tileMap)
{
    map = tileMap;
}

void Player::setTile(const glm::ivec2 &tile)
{
    const float tileSize = float(map->getTileSize());

    // Horizontally centered in the tile, feet on the bottom of the tile.
    // Also resets everything else, because this is also used when retrying
    posPlayer = glm::vec2(
        tile.x * tileSize + (tileSize - HITBOX_SIZE.x) / 2.f,
        (tile.y + 1) * tileSize - HITBOX_SIZE.y
    );

    velocityY = 0.f;
    onGround = false;
    coyoteMs = 0;
    invulnerableMs = 0;
    // If jump was pressed before, you have to release it and press it again to jump
    jumpWasPressed = true;
    facingRight = true;
    state = NORMAL;

    sprite->changeAnimation(STAND);
    sprite->setFlipX(facingRight);

    updateSpritePosition();
}

glm::vec2 Player::getCenter() const
{
    return posPlayer + HITBOX_SIZE / 2.f;
}

glm::vec4 Player::getBounds() const
{
    return glm::vec4(posPlayer, HITBOX_SIZE);
}

glm::ivec2 Player::getTile() const
{
    return map->toTile(getCenter());
}

bool Player::isAlive() const
{
    return state != DEAD;
}

bool Player::isDying() const
{
    return state == DYING || state == DEAD;
}

// Something touched him. Nothing happens if he's already dying or celebrating,
// in god mode, or still blinking after losing the heart
bool Player::hurt(bool byBomb)
{
    if (state != NORMAL || godMode || invulnerableMs > 0)
        return false;

    // The heart absorbs the hit and gives him 2 s to get away
    if (stats.heart)
    {
        stats.heart = false;
        invulnerableMs = INVULNERABLE_MS;
        Audio::instance().playSound(Sound::HEART_LOST);
        return false;
    }

    // Burning in the fire and being caught by an enemy look different
    state = DYING;
    sprite->changeAnimation(byBomb ? DEATH_BY_BOMB : DEATH_BY_MONSTER);
    Audio::instance().playSound(Sound::PLAYER_DEATH);
    return true;
}

// Used when time runs out: neither the heart nor god mode save you from the clock.
// No sound here, Scene plays the "time up" jingle instead
void Player::kill()
{
    if (state != NORMAL)
        return;

    invulnerableMs = 0;
    state = DYING;
    sprite->changeAnimation(DEATH_BY_MONSTER);
}

void Player::celebrate()
{
    if (state != NORMAL)
        return;

    state = CELEBRATING;
    invulnerableMs = 0;
    // The celebration frames look at the camera, so no flip
    sprite->changeAnimation(CELEBRATE);
    sprite->setFlipX(false);
}

// Every item improves one stat, up to its maximum
void Player::applyItem(ItemType type)
{
    switch (type)
    {
    case ItemType::BombUp:
        stats.maxBombs = std::min(stats.maxBombs + 1, MAX_BOMBS);
        break;
    case ItemType::FireUp:
        stats.fireRange = std::min(stats.fireRange + 1, MAX_FIRE);
        break;
    case ItemType::Heart:
        stats.heart = true;
        break;
    case ItemType::SpeedUp:
        stats.speedLevel = std::min(stats.speedLevel + 1, MAX_SPEED_LEVEL);
        break;
    }
}

const PlayerStats &Player::getStats() const
{
    return stats;
}

void Player::setStats(const PlayerStats &newStats)
{
    stats = newStats;
}

void Player::setGodMode(bool enabled)
{
    godMode = enabled;
}

bool Player::isGodMode() const
{
    return godMode;
}

// Picks the animation from what he's doing: in the air (going up or down),
// walking or standing
void Player::updateAnimation(bool walking)
{
    int wantedAnimation;

    if (!onGround)
        wantedAnimation = velocityY < 0.f ? JUMP : FALL;
    else if (walking)
        wantedAnimation = MOVE;
    else
        wantedAnimation = STAND;

    // Only when it changes: changeAnimation restarts from the first frame, so
    // calling it every frame would freeze the walking animation
    if (sprite->animation() != wantedAnimation)
        sprite->changeAnimation(wantedAnimation);

    // The atlas frames face left, so flip them when the player faces right.
    sprite->setFlipX(facingRight);
}

void Player::updateSpritePosition()
{
    // From the hitbox corner to the sprite corner (2 px up and to the left).
    // Rounded to whole pixels so that the pixel art does not shimmer
    sprite->setPosition(glm::round(posPlayer - HITBOX_OFFSET));
}
