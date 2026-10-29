#ifndef _TILE_MAP_INCLUDE
#define _TILE_MAP_INCLUDE


#include <string>
#include <vector>
#include <glm/glm.hpp>
#include "Texture.h"
#include "ShaderProgram.h"


// Class Tilemap loads a level exported from Tiled (.tmx with CSV encoding and an
// external .tsx tileset). Empty borders of the Tiled map are cropped away.
// All tiles are stored in a single VBO, which is rebuilt whenever a tile changes
// (for example when a bomb destroys a bush).


enum class TileType
{
	EMPTY,          // Background / decoration, entities move through it
	SOLID,          // Walls and floors
	DESTRUCTIBLE,   // Solid until an explosion removes it (bushes)
	GATE_CONTROL,   // Permanent cap; explosions trigger fence retraction
	DOOR            // Exit door, entities move through it
};


class TileMap
{

private:
	TileMap(const string &levelFile, ShaderProgram &program);

public:
	// Tile maps can only be created inside an OpenGL context
	static TileMap *createTileMap(const string &levelFile, ShaderProgram &program);

	~TileMap();

	void render() const;
	void update(int deltaTime);
	void free();

	bool isLoaded() const { return loaded; }
	int getTileSize() const { return tileSize; }
	glm::ivec2 getMapSize() const { return mapSize; }
	glm::ivec2 getPixelSize() const { return mapSize * tileSize; }

	TileType getTileType(int x, int y) const;
	// A tile is solid if it is a wall, a bush or it is blocked by a bomb
	bool isSolid(int x, int y) const;
	// True if the box (in pixels) overlaps any solid tile
	bool collides(const glm::vec2 &pos, const glm::vec2 &size) const;
	glm::ivec2 toTile(const glm::vec2 &pixel) const;

	// Removes a bush or starts retracting a fence, preserving its control.
	bool destroyTile(int x, int y);
	void placeDoor(int x, int y);
	bool findDoor(glm::ivec2 &tile) const;

	// Bombs block the tile they are in
	void setBlocked(int x, int y, bool blocked);

	// Access to the tilesheet so that other entities can draw tiles (the door)
	Texture &getTilesheet() { return tilesheet; }
	glm::vec2 getTileTexCoord(int tileId) const;
	glm::vec2 getTileTexSize() const;
	int getDoorTileId() const;

private:
	bool loadLevel(const string &levelFile);
	bool loadTileset(const string &tilesetFile);
	void prepareArrays();
	bool inside(int x, int y) const;

private:
	ShaderProgram *program;
	GLuint vao;
	GLuint vbo;
	GLint posLocation, texCoordLocation;
	int nTiles;
	bool loaded;

	glm::ivec2 mapSize;
	int tileSize;

	// Tileset layout (as described by the .tsx file)
	Texture tilesheet;
	int tilesetColumns, tilesetMargin, tilesetSpacing;

	// Tile ids are 0-based indices into the tileset (-1 = no tile)
	vector<int> map;
	vector<bool> flipped;
	vector<bool> blocked;
	struct RetractingGate
	{
		int x, controlY, bottomY, elapsedMs;
	};
	vector<RetractingGate> retractingGates;

};


#endif // _TILE_MAP_INCLUDE
