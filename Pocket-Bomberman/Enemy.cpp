#include <algorithm>
#include <cmath>

#include "Enemy.h"
#include "Gameplay.h"

namespace
{
    constexpr float SPRITE_SIZE = 16.f;
    const glm::vec2 HITBOX_SIZE(14.f, 14.f);
    const glm::vec2 HITBOX_OFFSET(1.f, 2.f);

    constexpr float GRAVITY = 700.f;
    constexpr float MAX_FALL_SPEED = 280.f;

    constexpr int DYING_MS = 700;

    // Rows of images/sprites.png with each enemy
    constexpr int WALKER_ROW = 5;
    constexpr int HOPPER_ROW = 6;
    constexpr int CHASER_ROW = 7;

    // Walker
    constexpr float WALKER_SPEED = 30.f;

    // Hopper
    constexpr float HOPPER_WAIT_MS = 1100.f;
    constexpr float HOPPER_JUMP_SPEED = 220.f;
    constexpr float HOPPER_SPEED = 50.f;

    // Chaser
    constexpr float CHASER_PATROL_SPEED = 22.f;
    constexpr float CHASER_RUN_SPEED = 58.f;
    constexpr float CHASER_SIGHT_X = 112.f;
    constexpr float CHASER_SIGHT_Y = 20.f;

    enum EnemyAnimation
    {
        MOVE,
        AIR,
        DEATH,

        ANIMATION_COUNT
    };
}

std::unique_ptr<Enemy> Enemy::create(Type type)
{
    switch (type)
    {
    case Type::HOPPER:
        return std::unique_ptr<Enemy>(new Hopper());
    case Type::CHASER:
        return std::unique_ptr<Enemy>(new Chaser());
    case Type::WALKER:
    default:
        return std::unique_ptr<Enemy>(new Walker());
    }
}

Enemy::Enemy(int atlasRow)
    : map(nullptr),
      position(0.f),
      velocity(0.f),
      direction(-1),
      onGround(false),
      hitWall(false),
      atlasRow(atlasRow),
      state(ALIVE),
      dyingMs(0)
{
}

void Enemy::init(Texture &atlas,
                 ShaderProgram &program,
                 TileMap *tileMap,
                 const glm::ivec2 &tile)
{
    map = tileMap;

    const float tileSize = float(map->getTileSize());
    position = glm::vec2(
        tile.x * tileSize + (tileSize - HITBOX_SIZE.x) / 2.f,
        (tile.y + 1) * tileSize - HITBOX_SIZE.y
    );

    sprite.reset(Sprite::createSprite(
        glm::vec2(SPRITE_SIZE),
        atlasCellSize(),
        &atlas,
        &program
    ));

    sprite->setNumberAnimations(ANIMATION_COUNT);

    sprite->setAnimationSpeed(MOVE, 6);
    for (int column : { 0, 1, 2, 1 })
        sprite->addKeyframe(MOVE, atlasCell(column, atlasRow));

    sprite->setAnimationSpeed(AIR, 1);
    sprite->addKeyframe(AIR, atlasCell(4, atlasRow));

    sprite->setAnimationSpeed(DEATH, 6);
    sprite->setAnimationLoop(DEATH, false);
    sprite->addKeyframe(DEATH, atlasCell(5, atlasRow));
    sprite->addKeyframe(DEATH, atlasCell(6, atlasRow));

    sprite->changeAnimation(MOVE);
    sprite->setPosition(glm::round(position - HITBOX_OFFSET));
}

void Enemy::update(int deltaTime, const glm::vec2 &playerCenter)
{
    sprite->update(deltaTime);

    if (state == DEAD)
        return;

    if (state == DYING)
    {
        dyingMs -= deltaTime;
        if (dyingMs <= 0)
            state = DEAD;
        return;
    }

    const float dt = std::min(deltaTime / 1000.f, 0.05f);

    think(dt, playerCenter);

    velocity.y = std::min(velocity.y + GRAVITY * dt, MAX_FALL_SPEED);

    hitWall = moveAndCollide(*map, position, HITBOX_SIZE, velocity.x * dt, 0);

    if (moveAndCollide(*map, position, HITBOX_SIZE, velocity.y * dt, 1))
    {
        onGround = velocity.y > 0.f;
        velocity.y = 0.f;
    }
    else
    {
        onGround = false;
    }

    const int wantedAnimation = onGround ? MOVE : AIR;
    if (sprite->animation() != wantedAnimation)
        sprite->changeAnimation(wantedAnimation);

    // Enemy frames look to the left
    sprite->setFlipX(direction > 0);
    sprite->setPosition(glm::round(position - HITBOX_OFFSET));
}

void Enemy::render()
{
    if (state == DEAD)
        return;

    // Blink at the end of the death animation
    if (state == DYING && dyingMs < DYING_MS / 2 && (dyingMs / 60) % 2 == 0)
        return;

    sprite->render();
}

void Enemy::kill()
{
    if (state != ALIVE)
        return;

    state = DYING;
    dyingMs = DYING_MS;
    sprite->changeAnimation(DEATH);
}

bool Enemy::isDying() const
{
    return state != ALIVE;
}

bool Enemy::isDead() const
{
    return state == DEAD;
}

glm::vec4 Enemy::getBounds() const
{
    return glm::vec4(position, HITBOX_SIZE);
}

bool Enemy::groundAhead(int dir) const
{
    const glm::vec2 foot(
        dir > 0 ? position.x + HITBOX_SIZE.x + 1.f : position.x - 1.f,
        position.y + HITBOX_SIZE.y + 1.f
    );
    const glm::ivec2 tile = map->toTile(foot);

    return map->isSolid(tile.x, tile.y);
}


// Walker: patrols its platform

Walker::Walker()
    : Enemy(WALKER_ROW)
{
}

void Walker::think(float dt, const glm::vec2 &playerCenter)
{
    if (hitWall || (onGround && !groundAhead(direction)))
        direction = -direction;

    velocity.x = direction * WALKER_SPEED;
}


// Hopper: waits on the floor and jumps towards the player

Hopper::Hopper()
    : Enemy(HOPPER_ROW),
      waitMs(HOPPER_WAIT_MS)
{
}

void Hopper::think(float dt, const glm::vec2 &playerCenter)
{
    if (onGround)
    {
        velocity.x = 0.f;
        waitMs -= dt * 1000.f;

        if (waitMs <= 0.f)
        {
            const glm::vec2 center(getBounds().x + getBounds().z / 2.f,
                                   getBounds().y + getBounds().w / 2.f);
            direction = playerCenter.x < center.x ? -1 : 1;
            velocity.y = -HOPPER_JUMP_SPEED;
            velocity.x = direction * HOPPER_SPEED;
            waitMs = HOPPER_WAIT_MS;
        }
    }
    else if (hitWall)
    {
        // Bounce against walls while in the air
        direction = -direction;
        velocity.x = direction * HOPPER_SPEED;
    }
}


// Chaser: runs after the player when he is at the same height and close enough

Chaser::Chaser()
    : Enemy(CHASER_ROW)
{
}

void Chaser::think(float dt, const glm::vec2 &playerCenter)
{
    const glm::vec2 center(getBounds().x + getBounds().z / 2.f,
                           getBounds().y + getBounds().w / 2.f);

    const bool seesPlayer =
        std::abs(playerCenter.y - center.y) < CHASER_SIGHT_Y &&
        std::abs(playerCenter.x - center.x) < CHASER_SIGHT_X;

    float speed = CHASER_PATROL_SPEED;

    if (seesPlayer)
    {
        direction = playerCenter.x < center.x ? -1 : 1;
        speed = CHASER_RUN_SPEED;
    }
    else if (hitWall)
    {
        direction = -direction;
    }

    // Never falls from its platform: waits at the edge or turns around
    if (onGround && !groundAhead(direction))
    {
        if (seesPlayer)
            speed = 0.f;
        else
            direction = -direction;
    }

    velocity.x = direction * speed;
}
