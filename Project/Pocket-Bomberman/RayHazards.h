#ifndef RAY_HAZARDS_H
#define RAY_HAZARDS_H

#include <memory>
#include <vector>
#include "Sprite.h"
#include "TileMap.h"

class Player;

// The rays between two emitter blocks. All of them follow the same cycle,
// which comes straight from the level clock (levelMs): shoot for 1.5 s,
// stop for 1.5 s, and charge up at the end of the pause. Since the cycle
// only depends on the time, we don't need any timer of our own.
class RayHazards
{
public:
	// Finds the emitter pairs of the new level
	void init(TileMap &map, ShaderProgram &program);
	void clear();
	// Is this tile inside a ray that's shooting? (bombs there explode)
	bool hitsTile(const glm::ivec2 &tile, int levelMs) const;
	void damagePlayer(Player &player, const glm::vec4 &playerBox, int tileSize, int levelMs) const;
	// Draws the emitter animation and, while shooting, the ray
	void render(const TileMap &map, int levelMs);

private:
	bool rayOn(int levelMs) const;

	std::vector<RayInfo> rays;
	std::unique_ptr<Sprite> raySprite;
};

#endif
