#include <iostream>
#include <fstream>
#include <sstream>
#include <cmath>
#include <cstdlib>
#include "TileMap.h"


using namespace std;


namespace
{
	// Tile ids (0-based, as in the Tiled tileset images/Mapa.png)
	constexpr int BACKGROUND_TILE = 90;
	constexpr int BUSH_TILE = 45;
	constexpr int DOOR_TILE = 60;
	constexpr int GATE_CONTROL_TILE = 46;
	constexpr int GATE_BODY_TILE = 75;
	constexpr int GATE_BOTTOM_TILE = 76;
	constexpr int GATE_STEP_MS = 100;

	// Emitter blocks (left and right) and the two frames of the ray between them.
	// The frames of the emitters are the next pairs of tiles of the same row
	constexpr int LEFT_EMITTER_TILE = 105;
	constexpr int RIGHT_EMITTER_TILE = 106;
	const int RAY_TILES[] = { 77, 78 };

	// The blue fence tiles are solid; only the background is passable.
	const int PASSABLE_TILES[] = { BACKGROUND_TILE };

	// Tiled stores flip flags in the highest bits of each tile id
	constexpr unsigned long FLIPPED_HORIZONTALLY = 0x80000000UL;
	constexpr unsigned long TILE_ID_MASK = 0x0FFFFFFFUL;

	string readFile(const string &fileName)
	{
		ifstream fin(fileName.c_str());
		if(!fin.is_open())
			return "";
		stringstream buffer;
		buffer << fin.rdbuf();
		return buffer.str();
	}

	// Returns the value of attribute 'name' inside the XML tag that starts at tagStart
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
	glEnable(GL_TEXTURE_2D);
	tilesheet.use();
	glBindVertexArray(vao);
	glEnableVertexAttribArray(posLocation);
	glEnableVertexAttribArray(texCoordLocation);
	glDrawArrays(GL_TRIANGLES, 0, 6 * nTiles);
	glDisable(GL_TEXTURE_2D);
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
	if(text.empty())
		return false;

	size_t mapTag = text.find("<map ");
	if(mapTag == string::npos)
		return false;
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
		unsigned long gid = strtoul(value.c_str(), NULL, 10);
		flips[i] = (gid & FLIPPED_HORIZONTALLY) != 0;
		gid &= TILE_ID_MASK;
		if(gid == 0)
			continue;
		int firstGid = firstGids[0];
		for(int first : firstGids)
			if(first <= int(gid) && first > firstGid)
				firstGid = first;
		tiles[i] = int(gid) - firstGid;
	}

	// Crop the empty borders of the map
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

int TileMap::getEmitterTileId(bool left, int frame) const
{
	return (left ? LEFT_EMITTER_TILE : RIGHT_EMITTER_TILE) + 2 * frame;
}

void TileMap::prepareArrays()
{
	vector<float> vertices;
	glm::vec2 texSize = getTileTexSize();
	// Tiny inset so that rounding never samples the spacing between tiles of the tileset
	glm::vec2 inset(0.05f / tilesheet.width(), 0.05f / tilesheet.height());

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

	int tile = map[y * mapSize.x + x];
	if(tile < 0)
		return TileType::SOLID;
	if(tile == BUSH_TILE)
		return TileType::DESTRUCTIBLE;
	if(tile == GATE_CONTROL_TILE)
		return TileType::GATE_CONTROL;
	if(tile == DOOR_TILE)
		return TileType::DOOR;
	for(int passable : PASSABLE_TILES)
		if(tile == passable)
			return TileType::EMPTY;

	return TileType::SOLID;
}

bool TileMap::isSolid(int x, int y) const
{
	TileType type = getTileType(x, y);
	if(type == TileType::SOLID || type == TileType::DESTRUCTIBLE || type == TileType::GATE_CONTROL)
		return true;

	return blocked[y * mapSize.x + x];
}

// Collision test for axis aligned bounding boxes against the grid
bool TileMap::collides(const glm::vec2 &pos, const glm::vec2 &size) const
{
	// The small epsilon makes a box that exactly touches a tile not collide with it
	int x0 = int(floor(pos.x / tileSize));
	int x1 = int(floor((pos.x + size.x - 0.01f) / tileSize));
	int y0 = int(floor(pos.y / tileSize));
	int y1 = int(floor((pos.y + size.y - 0.01f) / tileSize));

	for(int y = y0; y <= y1; y++)
		for(int x = x0; x <= x1; x++)
			if(isSolid(x, y))
				return true;

	return false;
}

glm::ivec2 TileMap::toTile(const glm::vec2 &pixel) const
{
	return glm::ivec2(int(floor(pixel.x / tileSize)), int(floor(pixel.y / tileSize)));
}

bool TileMap::destroyTile(int x, int y)
{
	const TileType type = getTileType(x, y);
	if(type != TileType::DESTRUCTIBLE && type != TileType::GATE_CONTROL)
		return false;

	if(type == TileType::GATE_CONTROL)
	{
		for(const auto &gate : retractingGates)
			if(gate.x == x && gate.controlY == y)
				return false;
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
		// No fence, or already retracted (only the bottom cap is left under the control)
		if(bottom <= y + 1)
			return false;
		retractingGates.push_back({ x, y, bottom, 0 });
		return true;
	}
	map[y * mapSize.x + x] = BACKGROUND_TILE;
	flipped[y * mapSize.x + x] = false;
	prepareArrays();

	return true;
}

void TileMap::update(int deltaTime)
{
	bool changed = false;
	for(auto it = retractingGates.begin(); it != retractingGates.end();)
	{
		it->elapsedMs += deltaTime;
		// The fence stops when the bottom cap is right below the control
		while(it->elapsedMs >= GATE_STEP_MS && it->bottomY > it->controlY + 1)
		{
			it->elapsedMs -= GATE_STEP_MS;
			const int index = it->bottomY * mapSize.x + it->x;
			map[index] = BACKGROUND_TILE;
			flipped[index] = false;
			--it->bottomY;
			// Move the bottom cap upwards along with the retracting fence.
			map[it->bottomY * mapSize.x + it->x] = GATE_BOTTOM_TILE;
			changed = true;
		}
		if(it->bottomY == it->controlY + 1)
			it = retractingGates.erase(it);
		else
			++it;
	}
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
	if(inside(x, y))
		blocked[y * mapSize.x + x] = value;
}
