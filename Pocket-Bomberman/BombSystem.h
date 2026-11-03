#ifndef BOMB_SYSTEM_H
#define BOMB_SYSTEM_H

#include <memory>
#include <random>
#include <vector>
#include "Bomb.h"
#include "Explosion.h"
#include "Item.h"

class Player;
class RayHazards;
class TileMap;

class BombSystem
{
public:
	void init();
	void clear();
	void place(TileMap &map, const Player &player, Texture &atlas, ShaderProgram &program);
	void update(int deltaTime, TileMap &map,
	            const RayHazards &rays, int levelMs, Texture &atlas, ShaderProgram &program);
	void removeFinishedExplosions();
	void renderItemsAndBombs(ShaderProgram &program);
	void renderExplosions(ShaderProgram &program);

	const std::vector<std::unique_ptr<Explosion>> &getExplosions() const { return explosions; }
	std::vector<std::unique_ptr<Item>> &getItems() { return items; }

private:
	void explode(const Bomb &bomb, TileMap &map, Texture &atlas, ShaderProgram &program);
	void spawnItem(const glm::ivec2 &tile, const TileMap &map, Texture &atlas, ShaderProgram &program);

	Texture explosionTexture;
	std::mt19937 random{std::random_device()()};
	std::vector<std::unique_ptr<Bomb>> bombs;
	std::vector<std::unique_ptr<Explosion>> explosions;
	std::vector<std::unique_ptr<Item>> items;
};

#endif
