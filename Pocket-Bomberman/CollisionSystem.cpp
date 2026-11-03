#include "CollisionSystem.h"
#include "Audio.h"
#include "BombSystem.h"
#include "Boss.h"
#include "Enemy.h"
#include "Gameplay.h"
#include "Player.h"
#include "RayHazards.h"

void CollisionSystem::update(bool playing, Player &player, const std::vector<std::unique_ptr<Enemy>> &enemies,
	            Boss *boss, const std::vector<std::unique_ptr<Projectile>> &projectiles,
	            BombSystem &bombs, const RayHazards &rays, int tileSize, int levelMs) const
{
	const auto &explosions = bombs.getExplosions();

	// Fire against enemies, the boss and the boss projectiles
	for(const auto &explosion : explosions)
	{
		for(auto &enemy : enemies)
			if(!enemy->isDying() && explosion->hits(enemy->getBounds()))
				enemy->kill();
		if(boss && explosion->hits(boss->getBounds()))
			boss->hit();
		for(auto &projectile : projectiles)
			if(explosion->hits(projectile->getBounds()))
				projectile->destroy();
	}

	if(!playing || player.isDying())
		return;

	glm::vec4 playerBox = player.getBounds();

	// Things that hurt the player
	for(const auto &explosion : explosions)
		if(explosion->hits(playerBox))
			player.hurt(true);

	for(const auto &enemy : enemies)
		if(!enemy->isDying() && overlaps(shrink(enemy->getBounds(), 1.f), playerBox))
			player.hurt(false);

	if(boss && !boss->isDying() && overlaps(boss->getBounds(), playerBox))
		player.hurt(false);

	rays.damagePlayer(player, playerBox, tileSize, levelMs);

	for(auto &projectile : projectiles)
		if(projectile->isAlive() && overlaps(projectile->getBounds(), playerBox))
		{
			player.hurt(false);
			projectile->destroy();
		}

	if(player.isDying())
		return;

	// Items
	auto &items = bombs.getItems();
	for(size_t i = 0; i < items.size(); )
	{
		if(overlaps(items[i]->bounds(), playerBox))
		{
			player.applyItem(items[i]->getType());
			Audio::instance().playSound(Sound::ITEM_GET);
			items.erase(items.begin() + i);
		}
		else
			i++;
	}
}
