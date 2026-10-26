#include <algorithm>
#include <cmath>

#include "Boss.h"
#include "Gameplay.h"
#include "Audio.h"

namespace
{
    // Projectiles
    constexpr float SEED_GRAVITY = 400.f;
    constexpr int PROJECTILE_LIFE_MS = 6000;
    const glm::vec2 PROJECTILE_SIZE(10.f, 10.f);
    // Seed: row 9, column 11
    // Bird: row 8, columns 8 to 10
    constexpr int SEED_COLUMN = 11;
    constexpr int SEED_ROW = 9;
    constexpr int BIRD_COLUMN = 8;
    constexpr int BIRD_ROW = 8;
    constexpr int BIRD_FRAMES = 3;
    constexpr float BIRD_SPEED = 70.f;
    constexpr float BIRD_WAVE = 30.f;

    // Boss: frames of 32x32 pixels starting at pixel (0, 128) of the atlas
    constexpr int BOSS_FRAME_SIZE = 32;
    constexpr int BOSS_ATLAS_Y = 128;
    const glm::vec2 BOSS_HITBOX_SIZE(24.f, 28.f);
    const glm::vec2 BOSS_HITBOX_OFFSET(4.f, 4.f);

    constexpr int MAX_HEALTH = 5;
    constexpr int ATTACK_MS = 2400;
    constexpr int BERSERK_ATTACK_MS = 1600;   // Low health
    // How long the boss remains visually locked in its attack animation 
    // before returning to its normal resting stance
    constexpr int ATTACK_POSE_MS = 500;
    constexpr int INVULNERABLE_MS = 1000;
    constexpr int DYING_MS = 2000;

    enum BossAnimation
    {
        IDLE,
        ATTACK,

        ANIMATION_COUNT
    };
}


Projectile::Projectile(Kind kind,
                       const glm::vec2 &position,
                       const glm::vec2 &velocity,
                       Texture &atlas,
                       ShaderProgram &program)
    : kind(kind),
      position(position),
      velocity(velocity),
      lifeMs(PROJECTILE_LIFE_MS),
      alive(true)
{
    sprite.reset(Sprite::createSprite(
        glm::vec2(float(ATLAS_CELL)),
        atlasCellSize(),
        &atlas,
        &program
    ));

    sprite->setNumberAnimations(1);
    if (kind == Kind::BIRD)
    {
        sprite->setAnimationSpeed(0, 8);
        for (int frame = 0; frame < BIRD_FRAMES; ++frame)
            sprite->addKeyframe(0, atlasCell(BIRD_COLUMN + frame, BIRD_ROW));
    }
    else
    {
        sprite->setAnimationSpeed(0, 1);
        sprite->addKeyframe(0, atlasCell(SEED_COLUMN, SEED_ROW));
    }
    sprite->changeAnimation(0);
    sprite->setFlipX(velocity.x > 0.f);
}

void Projectile::update(int deltaTime, const TileMap &map)
{
    if (!alive)
        return;

    sprite->update(deltaTime);

    // Prevent tunneling through walls
    const float dt = std::min(deltaTime / 1000.f, 0.05f);
    lifeMs -= deltaTime;

    if (kind == Kind::SEED)
    {
        velocity.y += SEED_GRAVITY * dt;
        position += velocity * dt;

        if (map.collides(position, PROJECTILE_SIZE))
            alive = false;
    }
    else
    {
        position.x += velocity.x * dt;
        position.y += std::sin(lifeMs / 250.f) * BIRD_WAVE * dt;
    }

    const glm::vec2 mapSize = glm::vec2(map.getPixelSize());
    // The projectile is alive until it hits a wall or goes out of bounds
    // If the projectile goes out of bounds on the left or top,
    // we need to make sure it is still considered alive until it has fully left the screen
    if (lifeMs <= 0 ||
        position.x < -ATLAS_CELL || position.x > mapSize.x ||
        position.y < -ATLAS_CELL || position.y > mapSize.y)
    {
        alive = false;
    }

    // Displace the position of the sprite so that its hitbox is centered on the projectile's position
    sprite->setPosition(glm::round(position - (glm::vec2(float(ATLAS_CELL)) - PROJECTILE_SIZE) / 2.f));
}

void Projectile::render()
{
    if (alive)
        sprite->render();
}

void Projectile::destroy()
{
    alive = false;
}

bool Projectile::isAlive() const
{
    return alive;
}

glm::vec4 Projectile::getBounds() const
{
    return glm::vec4(position, PROJECTILE_SIZE);
}


Boss::Boss()
    : atlas(nullptr),
      program(nullptr),
      map(nullptr),
      position(0.f),
      state(FIGHTING),
      health(MAX_HEALTH),
      attackTimerMs(ATTACK_MS),
      attackCount(0),
      poseMs(0),
      invulnerableMs(0),
      dyingMs(0)
{
}

void Boss::init(Texture &atlasTexture,
                ShaderProgram &shaderProgram,
                TileMap *tileMap,
                const glm::ivec2 &tile)
{
    atlas = &atlasTexture;
    program = &shaderProgram;
    map = tileMap;
    position = glm::vec2(tile * map->getTileSize());

    sprite.reset(Sprite::createSprite(
        glm::vec2(float(BOSS_FRAME_SIZE)),
        glm::vec2(BOSS_FRAME_SIZE / ATLAS_SIZE),
        atlas,
        program
    ));

    sprite->setNumberAnimations(ANIMATION_COUNT);
    sprite->setAnimationSpeed(IDLE, 2);
    sprite->addKeyframe(IDLE, atlasPixel(0 * BOSS_FRAME_SIZE, BOSS_ATLAS_Y));
    sprite->addKeyframe(IDLE, atlasPixel(1 * BOSS_FRAME_SIZE, BOSS_ATLAS_Y));
    sprite->setAnimationSpeed(ATTACK, 6);
    sprite->addKeyframe(ATTACK, atlasPixel(2 * BOSS_FRAME_SIZE, BOSS_ATLAS_Y));
    sprite->addKeyframe(ATTACK, atlasPixel(3 * BOSS_FRAME_SIZE, BOSS_ATLAS_Y));
    sprite->changeAnimation(IDLE);
    sprite->setPosition(position);
}

void Boss::update(int deltaTime,
                  const glm::vec2 &playerCenter,
                  std::vector<std::unique_ptr<Projectile>> &projectiles)
{
    sprite->update(deltaTime);

    if (state == DEFEATED)
        return;

    if (state == DYING)
    {
        dyingMs -= deltaTime;

        // Shakes while dying
        const float shake = (dyingMs / 50) % 2 == 0 ? -1.f : 1.f;
        sprite->setPosition(position + glm::vec2(shake, 0.f));

        if (dyingMs <= 0)
            state = DEFEATED;
        return;
    }

    if (invulnerableMs > 0)
        invulnerableMs -= deltaTime;

    if (poseMs > 0)
    {
        poseMs -= deltaTime;
        if (poseMs <= 0)
            sprite->changeAnimation(IDLE);
    }

    attackTimerMs -= deltaTime;
    if (attackTimerMs <= 0)
    {
        attack(playerCenter, projectiles);
        attackTimerMs = health <= 2 ? BERSERK_ATTACK_MS : ATTACK_MS;
    }

    // Looks at the player
    sprite->setFlipX(playerCenter.x > position.x + BOSS_FRAME_SIZE / 2.f);
}

void Boss::attack(const glm::vec2 &playerCenter,
                  std::vector<std::unique_ptr<Projectile>> &projectiles)
{
    sprite->changeAnimation(ATTACK);
    poseMs = ATTACK_POSE_MS;
    Audio::instance().playSound(Sound::BOSS_ATTACK);

    const glm::vec2 mouth = position + glm::vec2(BOSS_FRAME_SIZE / 2.f - 5.f, 6.f);

    if (attackCount % 3 == 2)
    {
        // A bird flies towards the player, at his height
        const float dir = playerCenter.x < mouth.x ? -1.f : 1.f;
        const glm::vec2 start(mouth.x, playerCenter.y - PROJECTILE_SIZE.y / 2.f);
        projectiles.emplace_back(new Projectile(
            Projectile::Kind::BIRD, start, glm::vec2(dir * BIRD_SPEED, 0.f), *atlas, *program));
    }
    else
    {
        // Three seeds that fall around the player
        const float flightTimes[] = { 0.8f, 1.0f, 1.2f };
        const float offsets[] = { -20.f, 0.f, 20.f };

        for (int i = 0; i < 3; ++i)
        {
            const float t = flightTimes[i];
            const glm::vec2 target = playerCenter + glm::vec2(offsets[i], 0.f);
            const glm::vec2 delta = target - mouth;
            const glm::vec2 velocity(delta.x / t, delta.y / t - 0.5f * SEED_GRAVITY * t);

            projectiles.emplace_back(new Projectile(
                Projectile::Kind::SEED, mouth, velocity, *atlas, *program));
        }
    }

    ++attackCount;
}

void Boss::render()
{
    if (state == DEFEATED)
        return;

    // Blinks after being hit and while dying
    if ((invulnerableMs > 0 || state == DYING) && ((invulnerableMs + dyingMs) / 70) % 2 == 0)
        program->setUniform4f("color", 1.f, 0.4f, 0.4f, 1.f);

    sprite->render();

    program->setUniform4f("color", 1.f, 1.f, 1.f, 1.f);
}

bool Boss::hit()
{
    if (state != FIGHTING || invulnerableMs > 0)
        return false;

    --health;
    invulnerableMs = INVULNERABLE_MS;
    Audio::instance().playSound(Sound::BOSS_HIT);

    if (health <= 0)
        kill();

    return true;
}

void Boss::kill()
{
    if (state != FIGHTING)
        return;

    health = 0;
    state = DYING;
    dyingMs = DYING_MS;
    invulnerableMs = 0;
    sprite->changeAnimation(IDLE);
}

bool Boss::isDying() const
{
    return state != FIGHTING;
}

bool Boss::isDefeated() const
{
    return state == DEFEATED;
}

int Boss::getHealth() const
{
    return health;
}

int Boss::getMaxHealth() const
{
    return MAX_HEALTH;
}

glm::vec4 Boss::getBounds() const
{
    return glm::vec4(position + BOSS_HITBOX_OFFSET, BOSS_HITBOX_SIZE);
}
