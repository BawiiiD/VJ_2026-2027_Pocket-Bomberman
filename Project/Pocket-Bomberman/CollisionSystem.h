#ifndef COLLISION_SYSTEM_H
#define COLLISION_SYSTEM_H

#include <memory>
#include <vector>

class BombSystem;
class Boss;
class Enemy;
class Player;
class Projectile;
class RayHazards;

// The rules for "who touches whom" once everything has moved this frame:
// fire against enemies, the boss and its projectiles; then everything that can
// hurt Bomberman; and finally the items he picks up.
// It doesn't store anything, it just looks at what Scene gives it.
class CollisionSystem
{
public:
	// playing is false during the end-of-level banners: the fire still works,
	// but Bomberman can't be hurt or pick things up
	void update(bool playing, Player &player, const std::vector<std::unique_ptr<Enemy>> &enemies,
	            Boss *boss, const std::vector<std::unique_ptr<Projectile>> &projectiles,
	            BombSystem &bombs, const RayHazards &rays, int tileSize, int levelMs) const;
};

#endif
