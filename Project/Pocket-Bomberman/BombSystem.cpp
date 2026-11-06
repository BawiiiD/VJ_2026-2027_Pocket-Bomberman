#include <algorithm>
#include <iostream>
#include "BombSystem.h"
#include "Audio.h"
#include "Gameplay.h"
#include "Player.h"
#include "RayHazards.h"

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
	itemDrops.clear();
}

void BombSystem::setItemDrop(const glm::ivec2 &tile, ItemType type)
{
	for(auto &drop : itemDrops)
		if(drop.tile == tile)
		{
			drop.type = type;
			return;
		}
	itemDrops.push_back({ tile, type });
}

// Bombs float in the air, so they don't fall with gravity. We also block their
// tile right away: whoever is inside can still walk out, but nobody can walk
// back in, and you can land on top of it (TileMap::collides handles that).
void BombSystem::place(TileMap &map, const Player &player, Texture &atlas, ShaderProgram &program)
{
	// We already have the max number of bombs out (Bomb Up raises it)
	if(int(bombs.size()) >= player.getStats().maxBombs)
		return;

	// Bombs always go in the tile where Bomberman's center is, so they're
	// aligned with the grid. isSolid is also true if there's a bomb there already
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
		// (bombs float, so you can drop one in the gap where the ray goes)
		if(rays.hitsTile(bomb->getTilePos(), levelMs))
			bomb->trigger();
	}

	// Explode bombs one by one, as each explosion can trigger other bombs.
	// After each explosion we start the search again from the beginning,
	// because erasing from the vector moves the indexes and the explosion may
	// have triggered bombs that we already passed. This way a whole chain
	// explodes in the same frame
	bool exploded = true;
	while(exploded)
	{
		exploded = false;
		for(size_t i = 0; i < bombs.size(); i++)
		{
			if(bombs[i]->ready())
			{
				// We take the bomb out of the vector BEFORE exploding it, so the
				// fire doesn't find this same bomb when it looks for other bombs
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

// Works out which tiles the fire reaches. The fire goes out from the bomb in
// the 4 directions, tile by tile, until it reaches its range or hits
// something that stops it.
void BombSystem::explode(const Bomb &bomb, TileMap &map, Texture &atlas, ShaderProgram &program)
{
	// For each direction: how to move, and which piece of fire to draw in the
	// middle of the arm and at its tip. With this table one loop does all four
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
			// getTileType ignores the blocked tiles, so a bomb's tile looks EMPTY
			// here and we find the bomb below
			TileType type = map.getTileType(tile.x, tile.y);
			// Walls stop the fire before them
			if(type == TileType::SOLID)
				break;

			// Fire on the spout makes its fence go up. The fence pieces are
			// SOLID, so hitting the fence itself does nothing
			if(type == TileType::GATE_CONTROL)
			{
				if(map.destroyTile(tile.x, tile.y))
					Audio::instance().playSound(Sound::DOOR_OPEN);
				break;
			}

			// Bushes break and stop the blast. That tile still gets a flame tip so it
			// looks like the fire reached it, but it doesn't hurt (damaging = false)
			if(type == TileType::DESTRUCTIBLE)
			{
				map.destroyTile(tile.x, tile.y);
				cells.push_back({ tile, direction.end, false });
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

// If the level file put an item in this bush, it appears now. The drop is
// removed from the list, so each bush gives its item only once
void BombSystem::spawnItem(const glm::ivec2 &tile, const TileMap &map, Texture &atlas, ShaderProgram &program)
{
	auto drop = std::find_if(itemDrops.begin(), itemDrops.end(),
		[&tile](const ItemDrop &candidate) { return candidate.tile == tile; });
	if(drop == itemDrops.end())
		return;

	items.emplace_back(new Item(tile, drop->type, map.getTileSize(), atlas, program));
	itemDrops.erase(drop);
}

// Called after the collisions, so a finished explosion still got its last chance to hit
void BombSystem::removeFinishedExplosions()
{
	explosions.erase(remove_if(explosions.begin(), explosions.end(),
		[](const std::unique_ptr<Explosion> &explosion) { return explosion->finished(); }), explosions.end());
}

void BombSystem::renderItemsAndBombs(ShaderProgram &program)
{
	for(auto &item : items)
		item->render();

	if(bombs.empty())
		return;

	// "recolor" repaints the sprite with one color, keeping its light and dark
	// parts (see texture.frag). The bombs get a blue-grey tone, and then we turn
	// it off again so nothing else is affected
	program.setUniform3f("recolor", 0.6f, 0.6f, 0.7f);
	for(auto &bomb : bombs)
		bomb->render();
	program.setUniform3f("recolor", 0.f, 0.f, 0.f);
}

void BombSystem::renderExplosions(ShaderProgram &program)
{
	if(explosions.empty())
		return;

	// The fire is drawn orange
	program.setUniform3f("recolor", 1.f, 0.7f, 0.f);
	for(auto &explosion : explosions)
		explosion->render();
	program.setUniform3f("recolor", 0.f, 0.f, 0.f);
}
