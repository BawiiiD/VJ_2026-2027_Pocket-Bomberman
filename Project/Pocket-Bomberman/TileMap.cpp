#include <algorithm>
#include <iostream>
#include <fstream>
#include <sstream>
#include <cmath>
#include <cstdlib>
#include "TileMap.h"


using namespace std;


namespace
{
	// Tile ids (0-based, as in the Tiled tileset images/Mapa.png). The tileset has
	// 15 columns, so the id of the tile in row r and column c is r * 15 + c
	constexpr int BACKGROUND_TILE = 90;
	constexpr int BUSH_TILE = 45;
	// A broken bush blinks 10 times, changing every 70 ms
	constexpr int BUSH_FLASH_STEP_MS = 70;
	constexpr int BUSH_FLASH_DURATION_MS = 10 * BUSH_FLASH_STEP_MS;
	constexpr int DOOR_TILE = 60;
	// The spout, the middle pieces of the fence below it and its bottom end
	constexpr int GATE_CONTROL_TILE = 46;
	constexpr int GATE_BODY_TILE = 75;
	constexpr int GATE_BOTTOM_TILE = 76;
	// The fence goes up one tile every 100 ms
	constexpr int GATE_STEP_MS = 100;

	// Emitter blocks (left and right) and the two frames of the ray between them.
	// The frames of the emitters are the next pairs of tiles of the same row
	constexpr int LEFT_EMITTER_TILE = 105;
	constexpr int RIGHT_EMITTER_TILE = 106;
	const int RAY_TILES[] = { 77, 78 };

	// Tiled stores whether a tile is mirrored in the highest bits of its id,
	// so we have to read the flag and then clear those bits to get the real id
	constexpr unsigned long FLIPPED_HORIZONTALLY = 0x80000000UL;
	constexpr unsigned long TILE_ID_MASK = 0x0FFFFFFFUL;

	// The whole file in a string. The .tmx and .tsx are small, so it's the easy way
	string readFile(const string &fileName)
	{
		ifstream fin(fileName.c_str());
		if(!fin.is_open())
			return "";
		stringstream buffer;
		buffer << fin.rdbuf();
		return buffer.str();
	}

	// Returns the value of attribute 'name' inside the XML tag that starts at tagStart.
	// It's not a real XML parser, but Tiled always writes the files the same way,
	// so looking for  name="  is enough
	string readAttribute(const string &text, size_t tagStart, const string &name)
	{
		size_t tagEnd = text.find('>', tagStart);
		string key = " " + name + "=\"";
		size_t pos = text.find(key, tagStart);
		if(pos == string::npos || pos > tagEnd)
			return "";
		pos += key.size();
		return text.substr(pos, text.find('"', pos) - pos);
	}

	int readIntAttribute(const string &text, size_t tagStart, const string &name)
	{
		return atoi(readAttribute(text, tagStart, name).c_str());
	}

	// "levels/map01.tmx" -> "levels/". Paths inside a .tmx are relative to the .tmx itself
	string directoryOf(const string &path)
	{
		size_t slash = path.find_last_of("/\\");
		if(slash == string::npos)
			return "";
		return path.substr(0, slash + 1);
	}
}


TileMap *TileMap::createTileMap(const string &levelFile, ShaderProgram &program)
{
	TileMap *map = new TileMap(levelFile, program);

	return map;
}


TileMap::TileMap(const string &levelFile, ShaderProgram &program)
{
	this->program = &program;
	vao = 0;
	vbo = 0;
	nTiles = 0;
	tileSize = 16;
	mapSize = glm::ivec2(0);
	// If loading fails the map stays empty and isLoaded() says so; Scene checks it
	loaded = loadLevel(levelFile);
	if(!loaded)
		cout << "Could not load level " << levelFile << endl;
	prepareArrays();
}

TileMap::~TileMap()
{
	free();
}


void TileMap::render() const
{
	if(nTiles == 0)
		return;
	// The whole map in one draw call
	glEnable(GL_TEXTURE_2D);
	tilesheet.use();
	glBindVertexArray(vao);
	glEnableVertexAttribArray(posLocation);
	glEnableVertexAttribArray(texCoordLocation);
	glDrawArrays(GL_TRIANGLES, 0, 6 * nTiles);
	glDisable(GL_TEXTURE_2D);

	if(bushFlashes.empty())
		return;

	// The broken bushes are drawn on top, half transparent, so they blink while
	// fading. That needs blending, so we turn it on and afterwards we leave
	// OpenGL exactly as it was, to not affect anything drawn after the map
	const bool blending = glIsEnabled(GL_BLEND);
	GLint srcRgb, dstRgb, srcAlpha, dstAlpha;
	glGetIntegerv(GL_BLEND_SRC_RGB, &srcRgb);
	glGetIntegerv(GL_BLEND_DST_RGB, &dstRgb);
	glGetIntegerv(GL_BLEND_SRC_ALPHA, &srcAlpha);
	glGetIntegerv(GL_BLEND_DST_ALPHA, &dstAlpha);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	for(const BushFlash &flash : bushFlashes)
	{
		const float alpha = (flash.elapsedMs / BUSH_FLASH_STEP_MS) % 2 == 0 ? 0.9f : 0.15f;
		program->setUniform4f("color", 1.f, 1.f, 1.f, alpha);
		bushFlashSprite->setPosition(glm::vec2(flash.tile * tileSize));
		bushFlashSprite->setFlipX(flash.flipped);
		bushFlashSprite->render();
	}
	// The sprite changed these uniforms; put back what the next things drawn expect
	program->setUniform4f("color", 1.f, 1.f, 1.f, 1.f);
	glm::mat4 modelview(1.f);
	program->setUniformMatrix4f("modelview", modelview);
	program->setUniform2f("texCoordDispl", 0.f, 0.f);
	glBlendFuncSeparate(srcRgb, dstRgb, srcAlpha, dstAlpha);
	if(!blending)
		glDisable(GL_BLEND);
}

void TileMap::free()
{
	if(vao != 0)
		glDeleteVertexArrays(1, &vao);
	if(vbo != 0)
		glDeleteBuffers(1, &vbo);
	vao = 0;
	vbo = 0;
}

bool TileMap::loadLevel(const string &levelFile)
{
	string text = readFile(levelFile);
	size_t mapTag = text.find("<map ");
	if(mapTag == string::npos)
		return false;
	// Size of the map in Tiled, before cropping
	int width = readIntAttribute(text, mapTag, "width");
	int height = readIntAttribute(text, mapTag, "height");
	tileSize = readIntAttribute(text, mapTag, "tilewidth");

	// All tilesets of our maps point to the same image, so we only need the
	// first one. Every tileset entry still has its own firstgid.
	vector<int> firstGids;
	string tilesetSource;
	for(size_t tag = text.find("<tileset "); tag != string::npos; tag = text.find("<tileset ", tag + 1))
	{
		firstGids.push_back(readIntAttribute(text, tag, "firstgid"));
		if(tilesetSource.empty())
			tilesetSource = readAttribute(text, tag, "source");
	}
	if(firstGids.empty() || !loadTileset(directoryOf(levelFile) + tilesetSource))
		return false;

	// The tiles are a list of numbers separated by commas, row by row
	size_t dataStart = text.find("<data encoding=\"csv\">");
	size_t dataEnd = text.find("</data>");
	if(dataStart == string::npos || dataEnd == string::npos)
		return false;
	dataStart = text.find('>', dataStart) + 1;

	vector<int> tiles(width * height, -1);
	vector<bool> flips(width * height, false);
	stringstream csv(text.substr(dataStart, dataEnd - dataStart));
	string value;
	for(int i = 0; i < width * height && getline(csv, value, ','); i++)
	{
		// Tiled calls it gid (global id): 0 means "no tile", and the ids of each
		// tileset start at its firstgid. We turn it into our 0-based id
		unsigned long gid = strtoul(value.c_str(), NULL, 10);
		flips[i] = (gid & FLIPPED_HORIZONTALLY) != 0;
		gid &= TILE_ID_MASK;
		if(gid == 0)
			continue;
		// The tile belongs to the tileset with the biggest firstgid that is <= gid
		int firstGid = firstGids[0];
		for(int first : firstGids)
			if(first <= int(gid) && first > firstGid)
				firstGid = first;
		tiles[i] = int(gid) - firstGid;
	}

	// In Tiled the map is often bigger than what we draw. We keep only the box
	// that has tiles, so the coordinates in the level files start at (0,0)
	glm::ivec2 minTile(width, height), maxTile(-1, -1);
	for(int j = 0; j < height; j++)
		for(int i = 0; i < width; i++)
			if(tiles[j * width + i] >= 0)
			{
				minTile = glm::min(minTile, glm::ivec2(i, j));
				maxTile = glm::max(maxTile, glm::ivec2(i, j));
			}
	if(maxTile.x < 0)
		return false;

	mapSize = maxTile - minTile + glm::ivec2(1);
	map.assign(mapSize.x * mapSize.y, -1);
	flipped.assign(mapSize.x * mapSize.y, false);
	blocked.assign(mapSize.x * mapSize.y, false);
	for(int j = 0; j < mapSize.y; j++)
		for(int i = 0; i < mapSize.x; i++)
		{
			int source = (j + minTile.y) * width + (i + minTile.x);
			map[j * mapSize.x + i] = tiles[source];
			flipped[j * mapSize.x + i] = flips[source];
		}

	return true;
}

bool TileMap::loadTileset(const string &tilesetFile)
{
	string text = readFile(tilesetFile);
	size_t tilesetTag = text.find("<tileset ");
	size_t imageTag = text.find("<image ");
	if(tilesetTag == string::npos || imageTag == string::npos)
	{
		cout << "Could not load tileset " << tilesetFile << endl;
		return false;
	}

	// What we need to find a tile in the image: how many per row, the border
	// around the image (margin) and the gap between tiles (spacing)
	tilesetColumns = readIntAttribute(text, tilesetTag, "columns");
	tilesetMargin = readIntAttribute(text, tilesetTag, "margin");
	tilesetSpacing = readIntAttribute(text, tilesetTag, "spacing");

	string image = directoryOf(tilesetFile) + readAttribute(text, imageTag, "source");
	if(!tilesheet.loadFromFile(image, TEXTURE_PIXEL_FORMAT_RGBA))
	{
		cout << "Could not load tileset image " << image << endl;
		return false;
	}
	tilesheet.setWrapS(GL_CLAMP_TO_EDGE);
	tilesheet.setWrapT(GL_CLAMP_TO_EDGE);
	tilesheet.setMinFilter(GL_NEAREST);
	tilesheet.setMagFilter(GL_NEAREST);

	return true;
}

// Texture coordinates of the top-left corner of a tile of the tileset.
// Each tile is tileSize pixels plus the spacing to the next one
glm::vec2 TileMap::getTileTexCoord(int tileId) const
{
	int column = tileId % tilesetColumns;
	int row = tileId / tilesetColumns;

	return glm::vec2(
		float(tilesetMargin + column * (tileSize + tilesetSpacing)) / tilesheet.width(),
		float(tilesetMargin + row * (tileSize + tilesetSpacing)) / tilesheet.height()
	);
}

glm::vec2 TileMap::getTileTexSize() const
{
	return glm::vec2(float(tileSize) / tilesheet.width(), float(tileSize) / tilesheet.height());
}

int TileMap::getDoorTileId() const
{
	return DOOR_TILE;
}

// Looks for every left emitter and checks that, after a line of empty tiles,
// there's a right emitter on the same row. Each pair is one ray
vector<RayInfo> TileMap::findRays() const
{
	vector<RayInfo> rays;

	for(int j = 0; j < mapSize.y; j++)
		for(int i = 0; i < mapSize.x; i++)
		{
			if(map[j * mapSize.x + i] != LEFT_EMITTER_TILE)
				continue;

			// The ray goes right through the empty tiles until the other emitter
			int length = 0;
			while(getTileType(i + 1 + length, j) == TileType::EMPTY)
				length++;
			if(length > 0 && inside(i + 1 + length, j) && map[j * mapSize.x + i + 1 + length] == RIGHT_EMITTER_TILE)
			{
				RayInfo ray = { glm::ivec2(i + 1, j), length };
				rays.push_back(ray);
			}
		}

	return rays;
}

int TileMap::getRayTileId(int frame) const
{
	return RAY_TILES[frame % 2];
}

// In the tileset the emitter frames come in pairs (left, right) one after
// the other, so frame n is two tiles further than frame n - 1
int TileMap::getEmitterTileId(bool left, int frame) const
{
	return (left ? LEFT_EMITTER_TILE : RIGHT_EMITTER_TILE) + 2 * frame;
}

// Builds one big list of vertices with a quad for every tile, and uploads it.
// Drawing the map is then a single draw call instead of one per tile
void TileMap::prepareArrays()
{
	vector<float> vertices;
	glm::vec2 texSize = getTileTexSize();
	// We shrink each tile's texture area a tiny bit. Without it, rounding errors
	// sometimes pick pixels from the gap between tiles and you see thin lines
	glm::vec2 inset(0.05f / tilesheet.width(), 0.05f / tilesheet.height());

	// Throw away the old VBO before making the new one
	free();
	nTiles = 0;
	for(int j=0; j<mapSize.y; j++)
	{
		for(int i=0; i<mapSize.x; i++)
		{
			int tile = map[j * mapSize.x + i];
			if(tile < 0)
				continue;

			// Non-empty tile
			nTiles++;
			glm::vec2 posTile(i * tileSize, j * tileSize);
			glm::vec2 texCoordTile[2];
			texCoordTile[0] = getTileTexCoord(tile) + inset;
			texCoordTile[1] = getTileTexCoord(tile) + texSize - inset;
			// A mirrored tile just has its left and right texture coordinates swapped
			if(flipped[j * mapSize.x + i])
				swap(texCoordTile[0].x, texCoordTile[1].x);
			// First triangle
			vertices.push_back(posTile.x); vertices.push_back(posTile.y);
			vertices.push_back(texCoordTile[0].x); vertices.push_back(texCoordTile[0].y);
			vertices.push_back(posTile.x + tileSize); vertices.push_back(posTile.y);
			vertices.push_back(texCoordTile[1].x); vertices.push_back(texCoordTile[0].y);
			vertices.push_back(posTile.x + tileSize); vertices.push_back(posTile.y + tileSize);
			vertices.push_back(texCoordTile[1].x); vertices.push_back(texCoordTile[1].y);
			// Second triangle
			vertices.push_back(posTile.x); vertices.push_back(posTile.y);
			vertices.push_back(texCoordTile[0].x); vertices.push_back(texCoordTile[0].y);
			vertices.push_back(posTile.x + tileSize); vertices.push_back(posTile.y + tileSize);
			vertices.push_back(texCoordTile[1].x); vertices.push_back(texCoordTile[1].y);
			vertices.push_back(posTile.x); vertices.push_back(posTile.y + tileSize);
			vertices.push_back(texCoordTile[0].x); vertices.push_back(texCoordTile[1].y);
		}
	}
	if(nTiles == 0)
		return;

	glGenVertexArrays(1, &vao);
	glBindVertexArray(vao);
	glGenBuffers(1, &vbo);
	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	glBufferData(GL_ARRAY_BUFFER, 24 * nTiles * sizeof(float), &vertices[0], GL_STATIC_DRAW);
	posLocation = program->bindVertexAttribute("position", 2, 4*sizeof(float), 0);
	texCoordLocation = program->bindVertexAttribute("texCoord", 2, 4*sizeof(float), (void *)(2*sizeof(float)));
}

bool TileMap::inside(int x, int y) const
{
	return x >= 0 && y >= 0 && x < mapSize.x && y < mapSize.y;
}

TileType TileMap::getTileType(int x, int y) const
{
	// Everything outside the map behaves as a wall
	if(!inside(x, y))
		return TileType::SOLID;

	// Only a few tile ids are special; anything else is a wall
	int tile = map[y * mapSize.x + x];
	if(tile == BUSH_TILE)
		return TileType::DESTRUCTIBLE;
	if(tile == GATE_CONTROL_TILE)
		return TileType::GATE_CONTROL;
	if(tile == DOOR_TILE)
		return TileType::DOOR;
	// The blue fence tiles are solid; only the background is passable.
	if(tile == BACKGROUND_TILE)
		return TileType::EMPTY;

	return TileType::SOLID;
}

// Unlike getTileType, this also counts the tiles that have a bomb in them
bool TileMap::isSolid(int x, int y) const
{
	TileType type = getTileType(x, y);
	if(type == TileType::SOLID || type == TileType::DESTRUCTIBLE || type == TileType::GATE_CONTROL)
		return true;

	return blocked[y * mapSize.x + x];
}

// Collision test for a box against the grid: we look at every tile the box
// covers and check if any of them is solid
bool TileMap::collides(const glm::vec2 &pos, const glm::vec2 &size, const glm::vec2 &previousPos) const
{
	// The small epsilon makes a box that exactly touches a tile not collide with it
	const glm::vec2 innerSize = size - glm::vec2(0.01f);
	const glm::ivec2 first = toTile(pos);
	const glm::ivec2 last = toTile(pos + innerSize);
	const glm::ivec2 previousFirst = toTile(previousPos);
	const glm::ivec2 previousLast = toTile(previousPos + innerSize);

	for(int y = first.y; y <= last.y; y++)
		for(int x = first.x; x <= last.x; x++)
		{
			const TileType type = getTileType(x, y);
			if(type == TileType::SOLID || type == TileType::DESTRUCTIBLE || type == TileType::GATE_CONTROL)
				return true;

			// Bombs are special: when you drop one you're standing inside it. If it
			// blocked you right away you'd be stuck, so a bomb tile only blocks the
			// characters that weren't already touching it before this move. That
			// also lets you land on it from above and use it as a step
			const bool wasInside = x >= previousFirst.x && x <= previousLast.x &&
			                       y >= previousFirst.y && y <= previousLast.y;
			if(blocked[y * mapSize.x + x] && !wasInside)
				return true;
		}

	return false;
}

glm::ivec2 TileMap::toTile(const glm::vec2 &pixel) const
{
	return glm::ivec2(int(floor(pixel.x / tileSize)), int(floor(pixel.y / tileSize)));
}

// What an explosion does to a tile: a bush breaks, a spout opens its fence.
// Walls are never touched
bool TileMap::destroyTile(int x, int y)
{
	const TileType type = getTileType(x, y);
	if(type != TileType::DESTRUCTIBLE && type != TileType::GATE_CONTROL)
		return false;

	if(type == TileType::GATE_CONTROL)
	{
		// Already opening, don't start it twice
		for(const auto &gate : retractingGates)
			if(gate.x == x && gate.controlY == y)
				return false;
		// Go down the fence to find its last tile
		int bottom = y;
		for(int row = y + 1; inside(x, row); ++row)
		{
			const int index = row * mapSize.x + x;
			const int tile = map[index];
			if(tile != GATE_BODY_TILE && tile != GATE_BOTTOM_TILE)
				break;
			bottom = row;
			if(tile == GATE_BOTTOM_TILE)
				break;
		}
		// No fence remains below the permanent control.
		if(bottom <= y)
			return false;
		// update() will make it go up little by little
		retractingGates.push_back({ x, y, bottom, 0 });
		return true;
	}
	// A bush: it disappears from the map right away (so the fire and the
	// characters can go through), but we keep blinking its picture for a moment
	// so it doesn't just vanish
	if(!bushFlashSprite)
	{
		bushFlashSprite.reset(Sprite::createSprite(glm::vec2(float(tileSize)),
		                                          getTileTexSize(), &tilesheet, program));
		bushFlashSprite->setFrame(getTileTexCoord(BUSH_TILE));
	}
	bushFlashes.push_back({ glm::ivec2(x, y), flipped[y * mapSize.x + x], 0 });
	map[y * mapSize.x + x] = BACKGROUND_TILE;
	flipped[y * mapSize.x + x] = false;
	prepareArrays();

	return true;
}

void TileMap::update(int deltaTime)
{
	// Broken bushes stop blinking after a while
	for(BushFlash &flash : bushFlashes)
		flash.elapsedMs += deltaTime;
	bushFlashes.erase(std::remove_if(bushFlashes.begin(), bushFlashes.end(),
		[](const BushFlash &flash) { return flash.elapsedMs >= BUSH_FLASH_DURATION_MS; }), bushFlashes.end());

	bool changed = false;
	for(auto it = retractingGates.begin(); it != retractingGates.end();)
	{
		it->elapsedMs += deltaTime;
		// Every step the lowest tile of the fence becomes sky and the bottom end
		// moves one tile up, until there's nothing left below the spout
		// (the spout itself always stays)
		while(it->elapsedMs >= GATE_STEP_MS && it->bottomY > it->controlY)
		{
			it->elapsedMs -= GATE_STEP_MS;
			const int index = it->bottomY * mapSize.x + it->x;
			map[index] = BACKGROUND_TILE;
			flipped[index] = false;
			--it->bottomY;
			// Move the bottom cap upwards along with the retracting fence.
			if(it->bottomY > it->controlY)
				map[it->bottomY * mapSize.x + it->x] = GATE_BOTTOM_TILE;
			changed = true;
		}
		if(it->bottomY == it->controlY)
			it = retractingGates.erase(it);
		else
			++it;
	}
	// The tiles changed, so the VBO has to be rebuilt (only once, even if
	// several fences moved this frame)
	if(changed)
		prepareArrays();
}

void TileMap::placeDoor(int x, int y)
{
	if(!inside(x, y))
		return;

	map[y * mapSize.x + x] = DOOR_TILE;
	flipped[y * mapSize.x + x] = false;
	prepareArrays();
}

bool TileMap::findDoor(glm::ivec2 &tile) const
{
	for(int j = 0; j < mapSize.y; j++)
		for(int i = 0; i < mapSize.x; i++)
			if(map[j * mapSize.x + i] == DOOR_TILE)
			{
				tile = glm::ivec2(i, j);
				return true;
			}

	return false;
}

void TileMap::setBlocked(int x, int y, bool value)
{
	blocked[y * mapSize.x + x] = value;
}
