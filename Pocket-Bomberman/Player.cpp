#include <algorithm>
#include <cmath>
#include <initializer_list>

#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include "Player.h"
#include "Game.h"
#include "Gameplay.h"

namespace
{
    // Size of the sprite on screen and of the hitbox used for collisions.
    // The hitbox is a bit smaller so that Bomberman fits through 1-tile gaps.
    constexpr float SPRITE_SIZE = 16.f;
    const glm::vec2 HITBOX_SIZE(12.f, 14.f);
    const glm::vec2 HITBOX_OFFSET(2.f, 2.f);

    // Physics constants for the player (pixels and seconds)
    constexpr float GRAVITY = 700.f;
    // A full jump rises about 55 px: enough to climb 3 tiles (48 px) but not 4
    constexpr float JUMP_SPEED = 285.f;
    constexpr float MAX_FALL_SPEED = 280.f;
    constexpr float PLAYER_SPEED = 64.f;
    constexpr float SPEED_PER_LEVEL = 12.f;

    // Jumping is still allowed a few ms after walking off a ledge
    constexpr int COYOTE_MS = 80;
    // Blinking time after the heart has been lost
    constexpr int INVULNERABLE_MS = 2000;

    constexpr int MAX_BOMBS = 8;
    constexpr int MAX_FIRE = 8;
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

    // Helper function to add frames (columns of the player row) to an animation
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
    // Damage animations are executed only once
    addFrames(DEATH_BY_BOMB, { 10, 11, 12 }, 6, false);
    addFrames(DEATH_BY_MONSTER, { 13, 14, 13, 14 }, 5, false);
    addFrames(CELEBRATE, { 6, 7 }, 4, true);

    sprite->changeAnimation(STAND);
    sprite->setFlipX(false);

    updateSpritePosition();
}

void Player::update(int deltaTime)
{
    if (!sprite || map == nullptr)
        return;

    sprite->update(deltaTime);

    if (state == DEAD)
        return;

    // Player is playing a death animation
    if (state == DYING)
    {
        if (sprite->animationFinished())
            state = DEAD;
        return;
    }

    if (invulnerableMs > 0)
        invulnerableMs -= deltaTime;

    // Limit the delta time to avoid large jumps in physics calculations
    const float dt = std::min(deltaTime / 1000.f, 0.05f);

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

        if (moveX != 0.f)
            walking = !moveAndCollide(*map, posPlayer, HITBOX_SIZE, moveX, 0);

        const bool jumpKey = game.getKey(GLFW_KEY_UP) || game.getKey(GLFW_KEY_Z);

        // Jump starts once per key press
        if (jumpKey && !jumpWasPressed && (onGround || coyoteMs > 0))
        {
            velocityY = -JUMP_SPEED;
            onGround = false;
            coyoteMs = 0;
        }

        // Releasing the key early makes a shorter jump
        if (!jumpKey && velocityY < -JUMP_SPEED * 0.4f)
            velocityY = -JUMP_SPEED * 0.4f;

        jumpWasPressed = jumpKey;
    }

    // Gravity and vertical movement
    velocityY = std::min(velocityY + GRAVITY * dt, MAX_FALL_SPEED);

    const bool wasOnGround = onGround;

    if (moveAndCollide(*map, posPlayer, HITBOX_SIZE, velocityY * dt, 1))
    {
        // Landed on the floor or hit the ceiling
        onGround = velocityY > 0.f;
        velocityY = 0.f;
    }
    else
    {
        onGround = false;
    }

    if (onGround)
        coyoteMs = COYOTE_MS;
    else if (wasOnGround && velocityY >= 0.f)
        coyoteMs = COYOTE_MS;
    else
        coyoteMs = std::max(0, coyoteMs - deltaTime);

    if (state == NORMAL)
        updateAnimation(walking);

    updateSpritePosition();
}

void Player::render()
{
    if (!sprite)
        return;

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
    const float tileSize = float(map != nullptr ? map->getTileSize() : 16);

    // Horizontally centered in the tile, feet on the floor of the tile
    posPlayer = glm::vec2(
        tile.x * tileSize + (tileSize - HITBOX_SIZE.x) / 2.f,
        (tile.y + 1) * tileSize - HITBOX_SIZE.y
    );

    velocityY = 0.f;
    onGround = false;
    coyoteMs = 0;
    invulnerableMs = 0;
    jumpWasPressed = true;
    facingRight = true;
    state = NORMAL;

    if (sprite)
        sprite->changeAnimation(STAND);

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
    if (map == nullptr)
        return glm::ivec2(0);

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

bool Player::hurt(bool byBomb)
{
    if (state != NORMAL || godMode || invulnerableMs > 0)
        return false;

    // The heart absorbs the hit
    if (stats.heart)
    {
        stats.heart = false;
        invulnerableMs = INVULNERABLE_MS;
        return false;
    }

    state = DYING;
    sprite->changeAnimation(byBomb ? DEATH_BY_BOMB : DEATH_BY_MONSTER);
    return true;
}

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
    sprite->changeAnimation(CELEBRATE);
    sprite->setFlipX(false);
}

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

void Player::updateAnimation(bool walking)
{
    int wantedAnimation;

    if (!onGround)
        wantedAnimation = velocityY < 0.f ? JUMP : FALL;
    else if (walking)
        wantedAnimation = MOVE;
    else
        wantedAnimation = STAND;

    if (sprite->animation() != wantedAnimation)
        sprite->changeAnimation(wantedAnimation);

    // Update the sprite's horizontal flip based on the player's facing direction
    sprite->setFlipX(!facingRight);
}

void Player::updateSpritePosition()
{
    if (!sprite)
        return;

    // Rounded to whole pixels so that the pixel art does not shimmer
    sprite->setPosition(glm::round(posPlayer - HITBOX_OFFSET));
}
