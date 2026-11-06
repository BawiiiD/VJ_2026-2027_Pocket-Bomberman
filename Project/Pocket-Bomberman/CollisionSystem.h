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

class CollisionSystem
{
public:
	void update(bool playing, Player &player, const std::vector<std::unique_ptr<Enemy>> &enemies,
	            Boss *boss, const std::vector<std::unique_ptr<Projectile>> &projectiles,
	            BombSystem &bombs, const RayHazards &rays, int tileSize, int levelMs) const;
};

#endif
