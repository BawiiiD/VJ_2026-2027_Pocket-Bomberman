#ifndef _TILE_MAP_INCLUDE
#define _TILE_MAP_INCLUDE


#include <string>
#include <vector>
#include <memory>
#include <glm/glm.hpp>
#include "Texture.h"
#include "ShaderProgram.h"
#include "Sprite.h"


// TileMap is the level itself: the grid of tiles we drew in Tiled.
// It loads the .tmx (with CSV data) and its .tsx tileset, and crops away the
// empty border that Tiled leaves around the map, so tile (0,0) is the first
// real tile. It also answers the questions everybody asks about the level:
// "is this tile solid?", "does this box hit a wall?", "where is the door?".
// All the tiles are drawn with a single VBO, which we rebuild only when a
// tile changes (a bush explodes, a fence goes up, the boss door appears).


// What a tile means for the game (the tile id says how it looks, this says how it behaves)
enum class TileType
{
	EMPTY,          // Sky / decoration, entities move through it
	SOLID,          // Walls and floors
	DESTRUCTIBLE,   // Solid until an explosion removes it (bushes)
	GATE_CONTROL,   // The spout on top of a fence: it never breaks, but fire on it opens the fence
	DOOR            // Exit door, entities move through it
};

// Ray between two emitter blocks: the empty tiles from start to start + (length - 1, 0)
struct RayInfo
{
	glm::ivec2 start;
	int length;
};


class TileMap
{

private:
	TileMap(const string &levelFile, ShaderProgram &program);

public:
	// Tile maps can only be created once the OpenGL context exists (they make a VBO)
	static TileMap *createTileMap(const string &levelFile, ShaderProgram &program);

	~TileMap();

	void render() const;
	// Animates the bushes that just broke and the fences that are going up
	void update(int deltaTime);
	void free();

	bool isLoaded() const { return loaded; }
	int getTileSize() const { return tileSize; }
	glm::ivec2 getMapSize() const { return mapSize; }
	glm::ivec2 getPixelSize() const { return mapSize * tileSize; }

	// Tiles outside the map count as SOLID, so nobody can leave it
	TileType getTileType(int x, int y) const;
	// A tile is solid if it is a wall, a bush or it is blocked by a bomb
	bool isSolid(int x, int y) const;
	// True if the box (in pixels) overlaps any solid tile. previousPos is where
	// the box was before moving, so it can walk out of a bomb it was already in
	bool collides(const glm::vec2 &pos, const glm::vec2 &size, const glm::vec2 &previousPos) const;
	// Which tile a pixel is in
	glm::ivec2 toTile(const glm::vec2 &pixel) const;

	// Removes a bush, or starts opening a fence if it's the spout on top of one.
	// Returns false if there was nothing to do
	bool destroyTile(int x, int y);
	// Boss level: the door appears when the boss dies
	void placeDoor(int x, int y);
	bool findDoor(glm::ivec2 &tile) const;

	// Bombs block their tile; the bomb system supplies in-bounds coordinates.
	// It's a separate layer on top of the tiles, so the tile itself never changes
	void setBlocked(int x, int y, bool blocked);

	// Access to the tileset, so other classes can draw tiles of it
	// (the closed door, the rays and the emitter animation)
	Texture &getTilesheet() { return tilesheet; }
	glm::vec2 getTileTexCoord(int tileId) const;
	glm::vec2 getTileTexSize() const;
	int getDoorTileId() const;

	// Rays shot between each pair of emitter blocks of the same row
	vector<RayInfo> findRays() const;
	int getRayTileId(int frame) const;
	// Emitter frames: 0 waiting, 1 to 3 charging, 4 shooting
	int getEmitterTileId(bool left, int frame) const;

private:
	bool loadLevel(const string &levelFile);
	bool loadTileset(const string &tilesetFile);
	// Builds the VBO with one quad per tile
	void prepareArrays();
	bool inside(int x, int y) const;

private:
	ShaderProgram *program;
	GLuint vao;
	GLuint vbo;
	GLint posLocation, texCoordLocation;
	int nTiles;
	bool loaded;

	// Size in tiles
	glm::ivec2 mapSize;
	int tileSize;

	// Tileset layout (as described by the .tsx file)
	Texture tilesheet;
	int tilesetColumns, tilesetMargin, tilesetSpacing;

	// One entry per tile, row by row. Tile ids are 0-based indices into the
	// tileset (-1 = no tile)
	vector<int> map;
	// Tiles that Tiled stored mirrored
	vector<bool> flipped;
	// Tiles with a bomb in them
	vector<bool> blocked;
	// A destroyed bush keeps blinking for a moment so it doesn't just vanish
	struct BushFlash
	{
		glm::ivec2 tile;
		bool flipped;
		int elapsedMs;
	};
	vector<BushFlash> bushFlashes;
	std::unique_ptr<Sprite> bushFlashSprite;
	// A fence going up: its bottom tile moves up one row every step
	struct RetractingGate
	{
		int x, controlY, bottomY, elapsedMs;
	};
	vector<RetractingGate> retractingGates;

};


#endif // _TILE_MAP_INCLUDE
