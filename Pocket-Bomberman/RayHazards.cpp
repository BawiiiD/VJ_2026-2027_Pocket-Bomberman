#include "RayHazards.h"
#include "Gameplay.h"
#include "Player.h"

namespace
{
	// Rays are on (deadly) and off for these times. At the end of the off time
	// the emitters play their charging animation
	constexpr int RAY_ON_MS = 1500;
	constexpr int RAY_OFF_MS = 1500;
	constexpr int RAY_CHARGE_MS = 600;
	constexpr int RAY_FRAME_MS = 100;
	// Emitter frames: waiting, charging (several) and shooting
	constexpr int EMITTER_WAIT_FRAME = 0;
	constexpr int EMITTER_CHARGE_FRAMES = 3;
	constexpr int EMITTER_SHOOT_FRAME = 4;
	// Part of the ray tile that hurts (the ray is drawn in rows 2 to 13)
	constexpr float RAY_HIT_TOP = 3.f;
	constexpr float RAY_HIT_HEIGHT = 10.f;
}

void RayHazards::init(TileMap &map, ShaderProgram &program)
{
	rays = map.findRays();
	raySprite.reset(Sprite::createSprite(
		glm::vec2(float(map.getTileSize())),
		map.getTileTexSize(),
		&map.getTilesheet(),
		&program
	));
}

void RayHazards::clear()
{
	rays.clear();
	raySprite.reset();
}

bool RayHazards::rayOn(int levelMs) const
{
	return levelMs % (RAY_ON_MS + RAY_OFF_MS) < RAY_ON_MS;
}

bool RayHazards::hitsTile(const glm::ivec2 &tile, int levelMs) const
{
	if(!rayOn(levelMs))
		return false;

	for(const RayInfo &ray : rays)
		if(tile.y == ray.start.y && tile.x >= ray.start.x && tile.x < ray.start.x + ray.length)
			return true;

	return false;
}

void RayHazards::damagePlayer(Player &player, const glm::vec4 &playerBox, int tileSize, int levelMs) const
{
	if(rayOn(levelMs))
	{
		for(const RayInfo &ray : rays)
		{
			glm::vec4 rayBox(ray.start.x * tileSize, ray.start.y * tileSize + RAY_HIT_TOP,
			                 ray.length * tileSize, RAY_HIT_HEIGHT);
			if(overlaps(rayBox, playerBox))
				player.hurt(true);
		}
	}
}

void RayHazards::render(const TileMap &map, int levelMs)
{
	// Rays: the emitters wait, charge and shoot. The ray is only drawn while shooting
	if(raySprite)
	{
		int cycleMs = RAY_ON_MS + RAY_OFF_MS;
		int rayTime = levelMs % cycleMs;
		int chargeStart = cycleMs - RAY_CHARGE_MS;

		int emitterFrame = EMITTER_WAIT_FRAME;
		if(rayOn(levelMs))
			emitterFrame = EMITTER_SHOOT_FRAME;
		else if(rayTime >= chargeStart)
			emitterFrame = 1 + (rayTime - chargeStart) * EMITTER_CHARGE_FRAMES / RAY_CHARGE_MS;

		int tileSize = map.getTileSize();
		for(const RayInfo &ray : rays)
		{
			// The emitters are tiles of the map, their other frames are drawn over them
			raySprite->setFrame(map.getTileTexCoord(map.getEmitterTileId(true, emitterFrame)));
			raySprite->setPosition(glm::vec2((ray.start - glm::ivec2(1, 0)) * tileSize));
			raySprite->render();
			raySprite->setFrame(map.getTileTexCoord(map.getEmitterTileId(false, emitterFrame)));
			raySprite->setPosition(glm::vec2((ray.start + glm::ivec2(ray.length, 0)) * tileSize));
			raySprite->render();

			if(!rayOn(levelMs))
				continue;

			raySprite->setFrame(map.getTileTexCoord(map.getRayTileId(levelMs / RAY_FRAME_MS)));
			for(int i = 0; i < ray.length; i++)
			{
				raySprite->setPosition(glm::vec2((ray.start + glm::ivec2(i, 0)) * tileSize));
				raySprite->render();
			}
		}
	}
}
