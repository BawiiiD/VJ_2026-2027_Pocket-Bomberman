#ifndef RAY_HAZARDS_H
#define RAY_HAZARDS_H

#include <memory>
#include <vector>
#include "Sprite.h"
#include "TileMap.h"

class Player;

class RayHazards
{
public:
	void init(TileMap &map, ShaderProgram &program);
	void clear();
	bool hitsTile(const glm::ivec2 &tile, int levelMs) const;
	void damagePlayer(Player &player, const glm::vec4 &playerBox, int tileSize, int levelMs) const;
	void render(const TileMap &map, int levelMs);

private:
	bool rayOn(int levelMs) const;

	std::vector<RayInfo> rays;
	std::unique_ptr<Sprite> raySprite;
};

#endif
