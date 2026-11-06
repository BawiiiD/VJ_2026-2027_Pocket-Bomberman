#ifndef BOMB_SYSTEM_H
#define BOMB_SYSTEM_H

#include <memory>
#include <vector>
#include "Bomb.h"
#include "Explosion.h"
#include "Item.h"

class Player;
class RayHazards;
class TileMap;

// Everything that has to do with bombs: the bombs on the map, their
// explosions (and chain reactions), and the items that the bushes leave.
// We took it out of Scene so Scene only has to deal with the rules of the level.
class BombSystem
{
public:
	// Loads the explosion texture (once, when the game starts)
	void init();
	// Removes everything, used when a level is unloaded
	void clear();
	// Says that the bush in this tile leaves this item. The drops are
	// registered again whenever a level is loaded, so retrying a level gives
	// the items back
	void setItemDrop(const glm::ivec2 &tile, ItemType type);
	// Drops a bomb where Bomberman is, if he still has bombs left
	void place(TileMap &map, const Player &player, Texture &atlas, ShaderProgram &program);
	void update(int deltaTime, TileMap &map,
	            const RayHazards &rays, int levelMs, Texture &atlas, ShaderProgram &program);
	void removeFinishedExplosions();
	// Two render functions because they go at different depths: items and bombs
	// behind the characters, and the fire in front of everything
	void renderItemsAndBombs(ShaderProgram &program);
	void renderExplosions(ShaderProgram &program);

	// CollisionSystem needs these to check the fire and the items against the characters

	const std::vector<std::unique_ptr<Explosion>> &getExplosions() const { return explosions; }
	std::vector<std::unique_ptr<Item>> &getItems() { return items; }

private:
	void explode(const Bomb &bomb, TileMap &map, Texture &atlas, ShaderProgram &program);
	void spawnItem(const glm::ivec2 &tile, const TileMap &map, Texture &atlas, ShaderProgram &program);

	Texture explosionTexture;
	// Bushes that still have an item inside (from the POWERUP lines of the level file)
	struct ItemDrop
	{
		glm::ivec2 tile;
		ItemType type;
	};
	std::vector<ItemDrop> itemDrops;
	std::vector<std::unique_ptr<Bomb>> bombs;
	std::vector<std::unique_ptr<Explosion>> explosions;
	std::vector<std::unique_ptr<Item>> items;
};

#endif
