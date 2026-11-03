#include <algorithm>
#include <iostream>
#include <iterator>
#include "BombSystem.h"
#include "Audio.h"
#include "Gameplay.h"
#include "Player.h"
#include "RayHazards.h"

namespace
{
	// Chance that a destroyed bush leaves an item (in %) and weight of each item
	constexpr int ITEM_CHANCE = 40;
	const double ITEM_WEIGHTS[] = { 3., 3., 1., 2. };   // BombUp, FireUp, Heart, SpeedUp
}

void BombSystem::init()
{
	if(!explosionTexture.loadFromFile("images/explosion.png", TEXTURE_PIXEL_FORMAT_RGBA))
		cout << "Could not load images/explosion.png" << endl;
	explosionTexture.setWrapS(GL_CLAMP_TO_EDGE);
	explosionTexture.setWrapT(GL_CLAMP_TO_EDGE);
	explosionTexture.setMinFilter(GL_NEAREST);
	explosionTexture.setMagFilter(GL_NEAREST);
}

void BombSystem::clear()
{
	bombs.clear();
	explosions.clear();
	items.clear();
}

void BombSystem::place(TileMap &map, const Player &player, Texture &atlas, ShaderProgram &program)
{
	if(int(bombs.size()) >= player.getStats().maxBombs)
		return;

	// Bombs are always aligned with the tiles
	glm::ivec2 tile = player.getTile();
	if(map.isSolid(tile.x, tile.y))
		return;

	bombs.emplace_back(new Bomb(tile, player.getStats().fireRange, map.getTileSize(), atlas, program));
	map.setBlocked(tile.x, tile.y, true);
	Audio::instance().playSound(Sound::BOMB_PLACE);
}

void BombSystem::update(int deltaTime, TileMap &map,
	            const RayHazards &rays, int levelMs, Texture &atlas, ShaderProgram &program)
{
	for(auto &item : items)
		item->update(deltaTime);

	for(auto &bomb : bombs)
	{
		bomb->update(deltaTime);

		// Bombs inside a ray explode as soon as it shoots
		if(rays.hitsTile(bomb->getTilePos(), levelMs))
			bomb->trigger();
	}

	// Explode bombs one by one, as each explosion can trigger other bombs
	bool exploded = true;
	while(exploded)
	{
		exploded = false;
		for(size_t i = 0; i < bombs.size(); i++)
		{
			if(bombs[i]->ready())
			{
				std::unique_ptr<Bomb> bomb = std::move(bombs[i]);
				bombs.erase(bombs.begin() + i);
				map.setBlocked(bomb->getTilePos().x, bomb->getTilePos().y, false);
				explode(*bomb, map, atlas, program);
				exploded = true;
				break;
			}
		}
	}

	for(auto &explosion : explosions)
		explosion->update(deltaTime);
}

void BombSystem::explode(const Bomb &bomb, TileMap &map, Texture &atlas, ShaderProgram &program)
{
	struct Direction
	{
		glm::ivec2 step;
		Explosion::Piece middle;
		Explosion::Piece end;
	};
	const Direction directions[] =
	{
		{ glm::ivec2(-1, 0), Explosion::Piece::HORIZONTAL, Explosion::Piece::END_LEFT },
		{ glm::ivec2(1, 0), Explosion::Piece::HORIZONTAL, Explosion::Piece::END_RIGHT },
		{ glm::ivec2(0, -1), Explosion::Piece::VERTICAL, Explosion::Piece::END_UP },
		{ glm::ivec2(0, 1), Explosion::Piece::VERTICAL, Explosion::Piece::END_DOWN }
	};

	vector<Explosion::Cell> cells;
	Explosion::Cell center = { bomb.getTilePos(), Explosion::Piece::CENTER };
	cells.push_back(center);

	for(const Direction &direction : directions)
	{
		for(int distance = 1; distance <= bomb.getRange(); distance++)
		{
			glm::ivec2 tile = bomb.getTilePos() + direction.step * distance;
			// getTileType ignores the blocked tiles, so solid bombs are found below
			TileType type = map.getTileType(tile.x, tile.y);
			if(type == TileType::SOLID)
				break;

			// Only the top control can retract a fence; the bars block fire.
			if(type == TileType::GATE_CONTROL)
			{
				if(map.destroyTile(tile.x, tile.y))
					Audio::instance().playSound(Sound::DOOR_OPEN);
				break;
			}

			// Bushes are destroyed and stop the fire, without drawing fire over them
			if(type == TileType::DESTRUCTIBLE)
			{
				map.destroyTile(tile.x, tile.y);
				spawnItem(tile, map, atlas, program);
				Audio::instance().playSound(Sound::BUSH_BREAK);
				break;
			}

			// Other bombs explode too (chain reaction) and stop the fire.
			// Their own explosion draws the center on that tile
			bool bombFound = false;
			for(auto &other : bombs)
				if(other->getTilePos() == tile)
				{
					other->trigger();
					bombFound = true;
				}
			if(bombFound)
				break;

			// The end piece is only drawn when the fire reaches its full range
			Explosion::Cell cell = { tile, distance == bomb.getRange() ? direction.end : direction.middle };
			cells.push_back(cell);
		}
	}

	explosions.emplace_back(new Explosion(cells, map.getTileSize(), explosionTexture, program));
	Audio::instance().playSound(Sound::EXPLOSION);
}

void BombSystem::spawnItem(const glm::ivec2 &tile, const TileMap &map, Texture &atlas, ShaderProgram &program)
{
	std::uniform_int_distribution<int> chance(0, 99);
	if(chance(random) >= ITEM_CHANCE)
		return;

	std::discrete_distribution<int> itemType(std::begin(ITEM_WEIGHTS), std::end(ITEM_WEIGHTS));
	items.emplace_back(new Item(tile, ItemType(itemType(random)), map.getTileSize(), atlas, program));
}

void BombSystem::removeFinishedExplosions()
{
	explosions.erase(remove_if(explosions.begin(), explosions.end(),
		[](const std::unique_ptr<Explosion> &explosion) { return explosion->finished(); }), explosions.end());
}

void BombSystem::renderItemsAndBombs()
{
	for(auto &item : items)
		item->render();
	for(auto &bomb : bombs)
		bomb->render();
}

void BombSystem::renderExplosions()
{
	for(auto &explosion : explosions)
		explosion->render();
}
