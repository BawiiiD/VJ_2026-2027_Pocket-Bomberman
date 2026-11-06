#include <algorithm>
#include <cmath>
#include <utility>

#include "Enemy.h"
#include "Gameplay.h"
#include "Audio.h"

namespace
{
    // Hitbox 14x14, centered in the 16 px sprite and with the feet at the bottom
    constexpr float SPRITE_SIZE = 16.f;
    const glm::vec2 HITBOX_SIZE(14.f, 14.f);
    const glm::vec2 HITBOX_OFFSET(1.f, 2.f);

    // Same physics as the player
    constexpr float GRAVITY = 700.f;
    constexpr float MAX_FALL_SPEED = 280.f;

    // How long a dead enemy stays on screen (it blinks in the second half)
    constexpr int DYING_MS = 700;

    // Rows of images/sprites.png with each enemy
    constexpr int CHASER_ROW = 5;
    constexpr int HOPPER_ROW = 6;
    constexpr int WALKER_ROW = 7;

    // Walker: about 2 tiles per second
    constexpr float WALKER_SPEED = 30.f;

    // Hopper: waits 1.1 s, then jumps about 2 tiles high towards the player
    constexpr float HOPPER_WAIT_MS = 1100.f;
    constexpr float HOPPER_JUMP_SPEED = 220.f;
    constexpr float HOPPER_SPEED = 50.f;

    // Chaser: slow patrol, and a run that is almost as fast as the player. It sees 7 tiles ahead
    constexpr float CHASER_PATROL_SPEED = 22.f;
    constexpr float CHASER_RUN_SPEED = 58.f;
    constexpr float CHASER_SIGHT_X = 112.f;

    enum EnemyAnimation
    {
        MOVE,
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

Enemy::Enemy(int atlasRow,
             std::vector<int> moveColumns,
             std::vector<int> deathColumns)
    : map(nullptr),
      position(0.f),
      velocity(0.f),
      direction(-1),
      onGround(false),
      hitWall(false),
      atlasRow(atlasRow),
      moveColumns(std::move(moveColumns)),
      deathColumns(std::move(deathColumns)),
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

    // Standing in its tile, centered, like the player
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

    // The move animation is also used in the air (the hopper opens and closes its
    // mouth while it jumps). The death animation plays once and stays on its last frame
    sprite->setAnimationSpeed(MOVE, 6);
    for (int column : moveColumns)
        sprite->addKeyframe(MOVE, atlasCell(column, atlasRow));

    sprite->setAnimationSpeed(DEATH, 6);
    sprite->setAnimationLoop(DEATH, false);
    for (int column : deathColumns)
        sprite->addKeyframe(DEATH, atlasCell(column, atlasRow));

    sprite->changeAnimation(MOVE);
    sprite->setPosition(glm::round(position - HITBOX_OFFSET));
}

void Enemy::update(int deltaTime, const glm::vec2 &playerCenter)
{
    sprite->update(deltaTime);

    // A dying enemy doesn't move, it only counts down until it can be removed
    if (state == DYING)
    {
        dyingMs -= deltaTime;
        if (dyingMs <= 0)
            state = DEAD;
        return;
    }

    const float dt = deltaTime / 1000.f;

    // First the enemy decides, then the physics are the same for everyone
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

    // Enemy frames look to the left, so we flip them when they go right
    sprite->setFlipX(direction > 0);
    sprite->setPosition(glm::round(position - HITBOX_OFFSET));
}

void Enemy::render()
{
    // Blink during the second half of the death time, before disappearing
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
    Audio::instance().playSound(Sound::ENEMY_DEATH);
}

// True for DYING and DEAD: from the moment it's hit it doesn't count anymore
// (it can't hurt you, and it's not one of the enemies left)
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

// Looks at the tile just below and in front of its feet. If it's not solid,
// the next step would be into the void. A tile with a bomb counts as floor
bool Enemy::groundAhead(int dir) const
{
    const glm::vec2 foot(
        dir > 0 ? position.x + HITBOX_SIZE.x + 1.f : position.x - 1.f,
        position.y + HITBOX_SIZE.y + 1.f
    );
    const glm::ivec2 tile = map->toTile(foot);

    return map->isSolid(tile.x, tile.y);
}


// Walker: patrols its platform, turning around at walls and edges.
// It ignores the player completely

Walker::Walker()
    : Enemy(WALKER_ROW, { 0, 1 }, { 2, 3, 4 })
{
}

void Walker::think(float dt, const glm::vec2 &playerCenter)
{
    // We only check for edges on the ground: in the air there's never floor ahead,
    // and it would keep turning around every frame while falling
    if (hitWall || (onGround && !groundAhead(direction)))
        direction = -direction;

    velocity.x = direction * WALKER_SPEED;
}


// Hopper: waits on the floor and jumps towards the player

Hopper::Hopper()
    : Enemy(HOPPER_ROW, { 0, 1 }, { 2, 3, 4 }),
      waitMs(HOPPER_WAIT_MS)
{
}

void Hopper::think(float dt, const glm::vec2 &playerCenter)
{
    if (onGround)
    {
        // Still while it waits for the next jump
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
        // Bounce against walls while in the air. moveAndCollide stops it but
        // doesn't change velocity.x, so we flip it ourselves
        direction = -direction;
        velocity.x = direction * HOPPER_SPEED;
    }
}


// Chaser: patrols slowly, and runs after the player when it sees him in front
// of it, on the same floor. It never falls from its platform

Chaser::Chaser()
    : Enemy(CHASER_ROW, { 0, 1, 2, 1 }, { 3, 4, 5 })
{
}

void Chaser::think(float dt, const glm::vec2 &playerCenter)
{
    const glm::vec2 center(getBounds().x + getBounds().z / 2.f,
                           getBounds().y + getBounds().w / 2.f);

    const float distanceX = playerCenter.x - center.x;
    // Player and enemy hitboxes are both 14 pixels tall
    const glm::vec2 feetOffset(0.f, HITBOX_SIZE.y / 2.f - 0.01f);
    const bool sameLevel =
        map->toTile(playerCenter + feetOffset).y == map->toTile(center + feetOffset).y;
    // It only looks forward: the player has to be on the side it's facing
    // (distanceX and direction with the same sign), on the same row and close
    const bool seesPlayer =
        distanceX * direction > 0.f && sameLevel &&
        std::abs(distanceX) < CHASER_SIGHT_X;

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

    // Never falls from its platform: while chasing it waits at the edge looking
    // at you, and while patrolling it turns around
    if (onGround && !groundAhead(direction))
    {
        if (seesPlayer)
            speed = 0.f;
        else
            direction = -direction;
    }

    velocity.x = direction * speed;
}
