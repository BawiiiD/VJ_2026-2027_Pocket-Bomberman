#include <algorithm>
#include <cmath>

#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include "Player.h"
#include "Game.h"

namespace
{
    // Size of each frame in the spritesheet
    constexpr int FRAME_WIDTH = 16;
    constexpr int FRAME_HEIGHT = 16;

    // Size of the player in pixels (scaled up from the frame size)
    constexpr int PLAYER_WIDTH = 32;
    constexpr int PLAYER_HEIGHT = 32;

    // Physics constants for the player
    constexpr float GRAVITY = 900.f;
    constexpr float JUMP_SPEED = 360.f;
    constexpr float PLAYER_SPEED = 120.f;

    // Animation frame rates for different player actions
    constexpr int WALKING_FPS = 8;
    constexpr int JUMP_FPS = 8;
    constexpr int MONSTER_HIT_FPS = 6;
    constexpr int BOMB_DEATH_FPS = 10;

    // Row 1: walking, 3 frames
    constexpr int MOVE_X = 0;
    constexpr int MOVE_Y = 0 * FRAME_HEIGHT;

    // Row 2: jumping, 3 frames
    constexpr int JUMP_X = 0;
    constexpr int JUMP_Y = 1 * FRAME_HEIGHT;

    // Row 4: dying by bomb, 4 frames
    constexpr int BOMB_DEATH_X = 0;
    constexpr int BOMB_DEATH_Y = 3 * FRAME_HEIGHT;

    // Row 5: hit by monster, 2 frames
    constexpr int MONSTER_HIT_X = 0;
    constexpr int MONSTER_HIT_Y = 4 * FRAME_HEIGHT;

    enum PlayerAnimation
    {
        STAND,
        MOVE,
        JUMP,
        HIT_BY_BOMB,
        HIT_BY_MONSTER,

        ANIMATION_COUNT
    };

    const glm::ivec2 PLAYER_SIZE(
        PLAYER_WIDTH,
        PLAYER_HEIGHT
    );
}

Player::Player()
{
    sprite = nullptr;
    map = nullptr;

    tileMapDispl = glm::ivec2(0);
    posPlayer = glm::vec2(0.f);

    velocityY = 0.f;
    moveSpeed = PLAYER_SPEED;

    onGround = false;
    jumpWasPressed = false;
    facingRight = true;

    alive = true;
    state = NORMAL;
}

Player::~Player()
{
    delete sprite;
}

void Player::init(const glm::ivec2 &tileMapPos, ShaderProgram &shaderProgram)
{
    tileMapDispl = tileMapPos;

    spritesheet.loadFromFile(
        "images/texture.jpg",
        TEXTURE_PIXEL_FORMAT_RGBA
    );

    spritesheet.setMinFilter(GL_NEAREST);
    spritesheet.setMagFilter(GL_NEAREST);

    const float textureWidth = static_cast<float>(spritesheet.width());
    const float textureHeight = static_cast<float>(spritesheet.height());

    sprite = Sprite::createSprite(
        glm::vec2(PLAYER_WIDTH, PLAYER_HEIGHT),
        glm::vec2(
            FRAME_WIDTH / textureWidth,
            FRAME_HEIGHT / textureHeight
        ),
        &spritesheet,
        &shaderProgram
    );

    sprite->setNumberAnimations(ANIMATION_COUNT);

    // Lambda function to calculate texture coordinates for a given frame in the spritesheet
    auto uv = [&](int x, int y)
    {
        return glm::vec2(
            x / textureWidth,
            y / textureHeight
        );
    };

    // Helper function to add frames to a specific animation
    auto addFrames = [&](int animation,
                         int startX,
                         int rowY,
                         int frameCount,
                         int framesPerSecond)
    {
        sprite->setAnimationSpeed(
            animation,
            framesPerSecond
        );

        for (int frame = 0; frame < frameCount; ++frame)
        {
            sprite->addKeyframe(
                animation,
                uv(
                    startX + frame * FRAME_WIDTH,
                    rowY
                )
            );
        }
    };

    // Row 0: standing, 1 frame
    addFrames(STAND, MOVE_X, MOVE_Y, 1, WALKING_FPS);

    // Row 1: walking, 3 frames
    addFrames(MOVE, MOVE_X, MOVE_Y, 3, WALKING_FPS);

    // Row 2: jumping, 3 frames
    addFrames(JUMP, JUMP_X, JUMP_Y, 3, JUMP_FPS);

    // Row 4: death by bomb, 4 frames
    addFrames(
        HIT_BY_BOMB,
        BOMB_DEATH_X,
        BOMB_DEATH_Y,
        4,
        BOMB_DEATH_FPS
    );

    // Row 5: hit by monster, 2 frames.
    addFrames(
        HIT_BY_MONSTER,
        MONSTER_HIT_X,
        MONSTER_HIT_Y,
        2,
        MONSTER_HIT_FPS
    );

    // Walk and jump animations loop continuously
    sprite->setAnimationLoop(STAND, true);
    sprite->setAnimationLoop(MOVE, true);
    sprite->setAnimationLoop(JUMP, true);

    // Damage animations are executed only once
    sprite->setAnimationLoop(HIT_BY_BOMB, false);
    sprite->setAnimationLoop(HIT_BY_MONSTER, false);

    sprite->changeAnimation(STAND);
    sprite->setFlipX(false);

    updateSpritePosition();
}

void Player::update(int deltaTime)
{
    if (sprite == nullptr || map == nullptr)
        return;

    sprite->update(deltaTime);

    // Player is playing a death animation
    if (state == DYING_BY_MONSTER ||
        state == DYING_BY_BOMB)
    {
        if (sprite->animationFinished())
        {
            alive = false;
            state = DEAD;
        }

        updateSpritePosition();
        return;
    }

    if (!alive)
        return;

    // Limit the delta time to avoid large jumps in physics calculations
    const float dt = std::min(
        deltaTime / 1000.f,
        0.05f
    );

    bool walking = false;

    // Horizontal movement
    if (Game::instance().getKey(GLFW_KEY_LEFT))
    {
        facingRight = false;

        const float previousX = posPlayer.x;
        posPlayer.x -= moveSpeed * dt;

        glm::ivec2 collisionPosition(
            static_cast<int>(std::floor(posPlayer.x)),
            static_cast<int>(std::floor(posPlayer.y))
        );

        if (map->collisionMoveLeft(
                collisionPosition,
                PLAYER_SIZE))
        {
            posPlayer.x = previousX;
        }
        else
        {
            walking = true;
        }
    }
    else if (Game::instance().getKey(GLFW_KEY_RIGHT))
    {
        facingRight = true;

        const float previousX = posPlayer.x;
        posPlayer.x += moveSpeed * dt;

        glm::ivec2 collisionPosition(
            static_cast<int>(std::floor(posPlayer.x)),
            static_cast<int>(std::floor(posPlayer.y))
        );

        if (map->collisionMoveRight(
                collisionPosition,
                PLAYER_SIZE))
        {
            posPlayer.x = previousX;
        }
        else
        {
            walking = true;
        }
    }

    const bool jumpKey =
        Game::instance().getKey(GLFW_KEY_UP);

    // Jump starts once per key press
    if (jumpKey && !jumpWasPressed && onGround)
    {
        velocityY = -JUMP_SPEED;
        onGround = false;
    }

    jumpWasPressed = jumpKey;

    // Gravity and vertical movement
    velocityY += GRAVITY * dt;

    const float previousY = posPlayer.y;
    posPlayer.y += velocityY * dt;

    glm::ivec2 collisionPosition(
        static_cast<int>(std::floor(posPlayer.x)),
        static_cast<int>(std::floor(posPlayer.y))
    );

    if (velocityY >= 0.f)
    {
        int correctedY = collisionPosition.y;

        if (map->collisionMoveDown(
                collisionPosition,
                PLAYER_SIZE,
                &correctedY))
        {
            posPlayer.y =
                static_cast<float>(correctedY);

            velocityY = 0.f;
            onGround = true;
        }
        else
        {
            onGround = false;
        }
    }
    else
    {
        if (map->collisionMoveUp(
                collisionPosition,
                PLAYER_SIZE))
        {
            posPlayer.y = previousY;
            velocityY = 0.f;
        }

        onGround = false;
    }

    updateAnimation(walking);
    updateSpritePosition();
}

void Player::render()
{
    if (sprite != nullptr)
        sprite->render();
}

void Player::setTileMap(TileMap *tileMap)
{
    map = tileMap;
}

void Player::setPosition(const glm::vec2 &pos)
{
    posPlayer = pos;
    velocityY = 0.f;
    onGround = false;

    updateSpritePosition();
}

bool Player::isAlive() const
{
    return alive;
}

void Player::hitByMonster()
{
    if (!alive || state != NORMAL)
        return;

    state = DYING_BY_MONSTER;
    sprite->changeAnimation(HIT_BY_MONSTER);
}

void Player::hitByBomb()
{
    if (!alive || state != NORMAL)
        return;

    state = DYING_BY_BOMB;
    sprite->changeAnimation(HIT_BY_BOMB);
}

void Player::updateAnimation(bool walking)
{
    int wantedAnimation;

    if (!onGround)
        wantedAnimation = JUMP;
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
    if (sprite == nullptr)
        return;

    sprite->setPosition(
        glm::vec2(tileMapDispl) + posPlayer
    );
}