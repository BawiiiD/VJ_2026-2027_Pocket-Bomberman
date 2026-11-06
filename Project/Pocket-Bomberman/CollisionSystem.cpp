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

	// Fire against enemies, the boss and the boss projectiles. This happens even
	// if Bomberman is dying, so a bomb can still take an enemy with you.
	// It runs every frame of the explosion: kill() ignores enemies that are
	// already dying, and the boss is invulnerable for a second after each hit,
	// so one explosion only counts once
	for(const auto &explosion : explosions)
	{
		for(auto &enemy : enemies)
			if(explosion->hits(enemy->getBounds()))
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

	// Things that hurt the player. We just report the hit: Player::hurt decides
	// if it really hurts (god mode, the heart, blinking after losing it...).
	// true means the fire, which uses a different death animation
	for(const auto &explosion : explosions)
		if(explosion->hits(playerBox))
			player.hurt(true);

	for(const auto &enemy : enemies)
		// Dying enemies don't hurt anymore. The 1 pixel shrink forgives a tiny
		// touch at the corners
		if(!enemy->isDying() && overlaps(shrink(enemy->getBounds(), 1.f), playerBox))
			player.hurt(false);

	if(boss && !boss->isDying() && overlaps(boss->getBounds(), playerBox))
		player.hurt(false);

	rays.damagePlayer(player, playerBox, tileSize, levelMs);

	for(auto &projectile : projectiles)
		if(projectile->isAlive() && overlaps(projectile->getBounds(), playerBox))
		{
			player.hurt(false);
			// The projectile is used up, even if the heart saved Bomberman
			projectile->destroy();
		}

	// If something just killed him, he doesn't get the item he was touching
	if(player.isDying())
		return;

	// Items
	// Picked-up items are erased while we go through the list, so the index only
	// moves forward when we didn't erase (the next item slides into position i)
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
