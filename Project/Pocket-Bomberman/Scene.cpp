#include <iostream>
#include <fstream>
#include <sstream>
#include <cmath>
#include <algorithm>
#include <glm/gtc/matrix_transform.hpp>
#include "Scene.h"
#include "Game.h"
#include "Gameplay.h"
#include "Audio.h"


namespace
{
	constexpr int NUM_LEVELS = 5;
	constexpr int START_LIVES = 3;

	// Height of the bar with the game information, at the bottom of the screen
	constexpr int HUD_HEIGHT = 16;
	constexpr int WORLD_VIEW_HEIGHT = VIEW_HEIGHT - HUD_HEIGHT;
	// The world is drawn 1.5 times bigger than the menus, so we see about
	// 213x149 game pixels of the level. 1.5 * 2 = 3 window pixels per game pixel,
	// a whole number, so the pixel art stays sharp
	constexpr float CAMERA_ZOOM = 1.5f;
	constexpr float CAMERA_WIDTH = VIEW_WIDTH / CAMERA_ZOOM;
	constexpr float CAMERA_HEIGHT = WORLD_VIEW_HEIGHT / CAMERA_ZOOM;

	// Time that messages stay on screen (long enough for their jingles)
	constexpr int STAGE_TITLE_MS = 1500;
	constexpr int LEVEL_CLEAR_MS = 4200;
	constexpr int AFTER_DEATH_MS = 1200;
	constexpr int TIME_UP_MS = 4200;
	constexpr int GAME_OVER_MS = 9200;
	constexpr int VICTORY_MS = 11200;

	const glm::vec3 BANNER_BACKGROUND_COLOR(0.10f, 0.10f, 0.10f);
	const glm::vec4 BANNER_TITLE_COLOR(1.f, 0.95f, 0.65f, 1.f);
	const glm::vec4 BANNER_SUBTITLE_COLOR(1.f, 0.95f, 0.65f, 1.f);

	// Below this time the clock blinks and a warning sounds
	constexpr int HURRY_UP_MS = 30000;

	// 8x8 HUD icons in images/sprites.png (pixel coordinates)
	const glm::ivec2 ICON_LIVES(0, 24);
	const glm::ivec2 ICON_SPEED(0, 16);
	const glm::ivec2 ICON_TIME(16, 24);
	const glm::ivec2 ICON_BOMB(8, 16);
	const glm::ivec2 ICON_HEART(16, 16);
	const glm::ivec2 ICON_FIRE(0, 48);

	// Contents of a levels/levelXX.txt file. We read it into this struct first,
	// and loadLevel then creates the real objects from it
	struct LevelInfo
	{
		string mapFile;
		int time = 180;
		glm::ivec2 player = glm::ivec2(1);
		vector<pair<Enemy::Type, glm::ivec2>> enemies;
		vector<pair<glm::ivec2, ItemType>> itemDrops;
		bool hasBoss = false;
		glm::ivec2 boss = glm::ivec2(0);
		glm::ivec2 door = glm::ivec2(0);
	};

	// Each line is a keyword and its values, like "WALKER 12 13". Empty lines
	// and lines starting with # are ignored. stringstream lets us read a line
	// with >> as if it were cin
	bool loadLevelInfo(const string &fileName, LevelInfo &info)
	{
		ifstream fin(fileName.c_str());
		if(!fin.is_open())
			return false;

		string line;
		int lineNumber = 0;
		while(getline(fin, line))
		{
			++lineNumber;
			stringstream sstream(line);
			string keyword;
			sstream >> keyword;
			if(keyword.empty() || keyword[0] == '#')
				continue;

			glm::ivec2 tile;
			if(keyword == "MAP")
				sstream >> info.mapFile;
			else if(keyword == "TIME")
				sstream >> info.time;
			else if(keyword == "PLAYER")
				sstream >> info.player.x >> info.player.y;
			else if(keyword == "WALKER" || keyword == "HOPPER" || keyword == "CHASER")
			{
				sstream >> tile.x >> tile.y;
				Enemy::Type type = Enemy::Type::WALKER;
				if(keyword == "HOPPER")
					type = Enemy::Type::HOPPER;
				else if(keyword == "CHASER")
					type = Enemy::Type::CHASER;
				info.enemies.push_back(make_pair(type, tile));
			}
			else if(keyword == "POWERUP")
			{
				// A wrong POWERUP line stops the level from loading, so a typo
				// in the file is found right away instead of silently losing the item
				string name;
				if(!(sstream >> tile.x >> tile.y >> name))
				{
					cout << fileName << ":" << lineNumber << ": expected POWERUP x y TYPE" << endl;
					return false;
				}
				ItemType type;
				if(name == "BOMBUP")
					type = ItemType::BombUp;
				else if(name == "FIREUP")
					type = ItemType::FireUp;
				else if(name == "HEART")
					type = ItemType::Heart;
				else if(name == "SPEEDUP")
					type = ItemType::SpeedUp;
				else
				{
					cout << fileName << ":" << lineNumber << ": unknown POWERUP type " << name << endl;
					return false;
				}
				info.itemDrops.push_back(make_pair(tile, type));
			}
			else if(keyword == "BOSS")
			{
				sstream >> info.boss.x >> info.boss.y;
				info.hasBoss = true;
			}
			else if(keyword == "DOOR")
				sstream >> info.door.x >> info.door.y;
		}

		return !info.mapFile.empty();
	}

	string levelFileName(int level)
	{
		stringstream name;
		name << "levels/level0" << level << ".txt";
		return name.str();
	}

	// Each world has its own music, and the boss level has the boss theme
	Music levelMusic(int level, bool hasBoss)
	{
		if(hasBoss)
			return Music::BOSS;

		const Music stages[] = { Music::STAGE1, Music::STAGE2, Music::STAGE3, Music::STAGE4 };
		return stages[glm::clamp(level, 1, 4) - 1];
	}

	// 125000 ms -> "2:05". It rounds up, so the clock only shows 0:00 when
	// the time is really over
	string formatTime(int ms)
	{
		int seconds = (ms + 999) / 1000;
		stringstream result;
		result << seconds / 60 << ":" << (seconds % 60 < 10 ? "0" : "") << seconds % 60;
		return result.str();
	}
}


Scene::Scene()
{
	texProgram = NULL;
	atlas = NULL;
	text = NULL;
	bigText = NULL;
	hasDoor = false;
	doorTile = glm::ivec2(0);
	bossDoorTile = glm::ivec2(0);
	currentLevel = 1;
	lives = START_LIVES;
	godMode = false;
	state = State::PLAYING;
	stateMs = 0;
	levelMs = 0;
	timeLeftMs = 0;
	timeUp = false;
	finished = true;
	doorWasOpen = false;
	timeWarned = false;
	camera = glm::vec2(0.f);
}

Scene::~Scene()
{
	shutdown();
}


void Scene::init(ShaderProgram &program, Texture &atlasTexture, Text &smallText, Text &largeText)
{
	texProgram = &program;
	atlas = &atlasTexture;
	text = &smallText;
	bigText = &largeText;

	bombSystem.init();

	// Two sprites reused for every HUD icon: one 8x8 and one 16x16
	smallIcon.reset(Sprite::createSprite(glm::vec2(8.f), glm::vec2(8.f / ATLAS_SIZE), atlas, texProgram));
	icon.reset(Sprite::createSprite(glm::vec2(16.f), atlasCellSize(), atlas, texProgram));
}

void Scene::shutdown()
{
	unloadLevel();
	smallIcon.reset();
	icon.reset();
}

// PLAY in the menu: a new game, from level 1 with 3 lives and no items
void Scene::startGame()
{
	lives = START_LIVES;
	levelStartStats = PlayerStats();
	godMode = false;
	finished = false;
	loadLevel(1);
}

bool Scene::loadLevel(int level)
{
	LevelInfo info;
	if(!loadLevelInfo(levelFileName(level), info))
	{
		cout << "Could not load " << levelFileName(level) << endl;
		finished = true;
		return false;
	}

	// Read the file first: if it's wrong we keep the current level instead of
	// ending up with half of one
	unloadLevel();
	currentLevel = level;

	map.reset(TileMap::createTileMap(info.mapFile, *texProgram));
	if(!map->isLoaded())
	{
		finished = true;
		return false;
	}

	// Every POWERUP has to be on a bush; if not, it's a mistake in the level file
	for(const auto &drop : info.itemDrops)
	{
		const glm::ivec2 &tile = drop.first;
		if(map->getTileType(tile.x, tile.y) != TileType::DESTRUCTIBLE)
		{
			cout << levelFileName(level) << ": POWERUP at (" << tile.x << ", " << tile.y
			     << ") must be on a destructible bush" << endl;
			finished = true;
			return false;
		}
		bombSystem.setItemDrop(tile, drop.second);
	}

	player.reset(new Player());
	player->init(*atlas, *texProgram);
	player->setTileMap(map.get());
	player->setTile(info.player);
	// Items and god mode carry over from the last level (or from the last try)
	player->setStats(levelStartStats);
	player->setGodMode(godMode);

	for(const auto &enemyInfo : info.enemies)
	{
		std::unique_ptr<Enemy> enemy = Enemy::create(enemyInfo.first);
		enemy->init(*atlas, *texProgram, map.get(), enemyInfo.second);
		enemies.push_back(std::move(enemy));
	}

	if(info.hasBoss)
	{
		boss.reset(new Boss());
		boss->init(*atlas, *texProgram, map.get(), info.boss);
	}

	// The door is a tile of the map. In the boss level there isn't one yet, and
	// the DOOR line of the file says where it will appear
	hasDoor = map->findDoor(doorTile);
	bossDoorTile = info.door;
	doorSprite.reset(Sprite::createSprite(
		glm::vec2(float(map->getTileSize())),
		map->getTileTexSize(),
		&map->getTilesheet(),
		texProgram
	));
	// An extra door sprite, to draw it darker while it's closed
	doorSprite->setFrame(map->getTileTexCoord(map->getDoorTileId()));
	updateDoor();

	rayHazards.init(*map, *texProgram);

	timeLeftMs = info.time * 1000;
	timeUp = false;
	// So the warning and the door sound don't play just because the level loaded
	timeWarned = timeLeftMs <= HURRY_UP_MS;
	doorWasOpen = hasDoor && doorOpen();
	levelMs = 0;
	changeState(State::PLAYING);
	updateCamera();

	// After dying the music was stopped, so it starts again from the beginning
	Audio::instance().playMusic(levelMusic(level, info.hasBoss));

	return true;
}

// The map is deleted last because everything else keeps a pointer to it
void Scene::unloadLevel()
{
	bombSystem.clear();
	enemies.clear();
	projectiles.clear();
	boss.reset();
	player.reset();
	doorSprite.reset();
	rayHazards.clear();
	map.reset();
}

bool Scene::isFinished() const
{
	return finished;
}

// Always change state through here, so the state timer starts from 0
void Scene::changeState(State newState)
{
	state = newState;
	stateMs = 0;
}

// Keys that do something once per press. Walking and jumping are read by the
// Player every frame instead
void Scene::keyPressed(int key)
{
	if(finished)
		return;

	// ENTER or SPACE skip the game over and victory screens
	if((state == State::GAME_OVER || state == State::VICTORY) && (key == GLFW_KEY_ENTER || key == GLFW_KEY_SPACE))
	{
		finished = true;
		return;
	}

	switch(key)
	{
	case GLFW_KEY_SPACE:
	case GLFW_KEY_X:
		if(state == State::PLAYING && !player->isDying())
			bombSystem.place(*map, *player, *atlas, *texProgram);
		break;
	case GLFW_KEY_G:
		godMode = !godMode;
		player->setGodMode(godMode);
		break;
	case GLFW_KEY_K:
		killAllEnemies();
		break;
	case GLFW_KEY_1:
	case GLFW_KEY_2:
	case GLFW_KEY_3:
	case GLFW_KEY_4:
	case GLFW_KEY_5:
		// Jump to a level keeping the current lives and items.
		// key - GLFW_KEY_0 works because the number keys have consecutive codes
		if(state == State::PLAYING || state == State::LEVEL_CLEAR)
		{
			levelStartStats = player->getStats();
			loadLevel(key - GLFW_KEY_0);
		}
		break;
	}
}

void Scene::update(int deltaTime)
{
	if(finished)
		return;

	stateMs += deltaTime;
	levelMs += deltaTime;
	map->update(deltaTime);

	// 1. Everybody moves. The world keeps going during the banners too (enemies
	// walk, explosions finish), only the player's control and the clock stop
	glm::vec2 playerCenter = player->getCenter();

	player->update(deltaTime);
	for(auto &enemy : enemies)
		enemy->update(deltaTime, playerCenter);
	if(boss)
		boss->update(deltaTime, playerCenter, projectiles);
	for(auto &projectile : projectiles)
		projectile->update(deltaTime, *map);
	bombSystem.update(deltaTime, *map, rayHazards, levelMs, *atlas, *texProgram);

	// 2. Now that everything moved, check who touches whom
	collisionSystem.update(state == State::PLAYING, *player, enemies, boss.get(), projectiles,
	                       bombSystem, rayHazards, map->getTileSize(), levelMs);

	// Exit door: the player's center has to be inside the door tile, so just
	// touching it isn't enough, you have to walk in
	if(state == State::PLAYING && !player->isDying() && hasDoor && doorOpen() &&
	   map->toTile(player->getCenter()) == doorTile)
	{
		player->celebrate();
		changeState(State::LEVEL_CLEAR);
		Audio::instance().playMusic(Music::CLEAR, false);
	}
	// 3. Remove finished entities before rendering and the next update.
	// We do it after the collisions, so nothing is deleted while those loops
	// are still going through the lists. erase + remove_if is the usual C++
	// way to remove every element that matches a condition
	enemies.erase(remove_if(enemies.begin(), enemies.end(),
		[](const std::unique_ptr<Enemy> &enemy) { return enemy->isDead(); }), enemies.end());
	projectiles.erase(remove_if(projectiles.begin(), projectiles.end(),
		[](const std::unique_ptr<Projectile> &projectile) { return !projectile->isAlive(); }), projectiles.end());
	bombSystem.removeFinishedExplosions();

	updateDoor();

	// 4. The rules of the current state
	switch(state)
	{
	case State::PLAYING:
		timeLeftMs -= deltaTime;
		if(timeLeftMs <= 0)
		{
			timeLeftMs = 0;
			timeUp = true;
			// kill() and not hurt(): the heart doesn't save you from the clock
			player->kill();
			Audio::instance().playMusic(Music::TIME_UP, false);
		}
		if(!timeWarned && timeLeftMs <= HURRY_UP_MS)
		{
			timeWarned = true;
			Audio::instance().playSound(Sound::TIME_WARNING);
		}
		if(player->isDying())
		{
			// Silence for the death sound (when time is up, the jingle plays instead)
			if(!timeUp)
				Audio::instance().stopMusic();
			changeState(State::PLAYER_DEAD);
		}
		break;

	case State::LEVEL_CLEAR:
		if(stateMs >= LEVEL_CLEAR_MS)
		{
			// Keep the items for the next level
			levelStartStats = player->getStats();
			if(currentLevel == NUM_LEVELS)
			{
				changeState(State::VICTORY);
				Audio::instance().playMusic(Music::VICTORY, false);
			}
			else if(!loadLevel(currentLevel + 1))
				return;
		}
		break;

	case State::PLAYER_DEAD:
		// Wait for the death animation AND a bit more, so you can see what happened
		if(!player->isAlive() && stateMs >= (timeUp ? TIME_UP_MS : AFTER_DEATH_MS))
		{
			--lives;
			if(lives <= 0)
			{
				changeState(State::GAME_OVER);
				Audio::instance().playMusic(Music::GAME_OVER, false);
			}
			else
				// Same level again, with the items from when it started
				if(!loadLevel(currentLevel))
					return;
		}
		break;

	case State::GAME_OVER:
		if(stateMs >= GAME_OVER_MS)
			finished = true;
		break;

	case State::VICTORY:
		if(stateMs >= VICTORY_MS)
			finished = true;
		break;
	}

	// Last, so the camera uses where the player ended up this frame
	updateCamera();
}

void Scene::updateDoor()
{
	// In the boss level the door appears when the boss is defeated
	if(!hasDoor && boss && boss->isDefeated())
	{
		map->placeDoor(bossDoorTile.x, bossDoorTile.y);
		doorTile = bossDoorTile;
		hasDoor = true;
	}

	doorSprite->setPosition(glm::vec2(doorTile * map->getTileSize()));

	// Sound when the door opens (all the enemies defeated). We compare with last
	// frame so it only plays at the moment it opens
	bool open = hasDoor && doorOpen();
	if(open && !doorWasOpen)
		Audio::instance().playSound(Sound::DOOR_OPEN);
	doorWasOpen = open;
}

// K cheat
void Scene::killAllEnemies()
{
	for(auto &enemy : enemies)
		enemy->kill();
	if(boss)
		boss->kill();
	projectiles.clear();
}

// Enemies that are still alive (a dying one doesn't count anymore). The boss
// counts as one more
int Scene::enemiesLeft() const
{
	int count = 0;
	for(const auto &enemy : enemies)
		if(!enemy->isDying())
			count++;
	if(boss && !boss->isDying())
		count++;

	return count;
}

// In the boss level we also wait until its death animation is over
bool Scene::doorOpen() const
{
	return enemiesLeft() == 0 && (!boss || boss->isDefeated());
}

void Scene::updateCamera()
{
	glm::vec2 view(CAMERA_WIDTH, CAMERA_HEIGHT);
	glm::vec2 mapSize(map->getPixelSize());
	// Where the camera would have to be to have the player in the middle
	glm::vec2 target = player->getCenter() - view / 2.f;

	for(int axis = 0; axis < 2; axis++)
	{
		// Small maps are centered, big ones follow the player. The clamp stops
		// the camera at the edges so we never see outside the map
		if(mapSize[axis] <= view[axis])
			camera[axis] = -(view[axis] - mapSize[axis]) / 2.f;
		else
			camera[axis] = glm::clamp(target[axis], 0.f, mapSize[axis] - view[axis]);
	}

	// Align to screen pixels so the zoomed camera can move one screen pixel at a time.
	// A camera between pixels would make the whole map shimmer
	constexpr float screenPixelsPerWorldPixel = CAMERA_ZOOM * PIXEL_SCALE;
	camera = glm::round(camera * screenPixelsPerWorldPixel) / screenPixelsPerWorldPixel;
}

void Scene::render()
{
	if(!map)
		return;

	Game &game = Game::instance();
	glm::mat4 projection, modelview(1.0f);

	// The screen is split in two with glViewport: the world on top and the HUD
	// bar at the bottom. Nothing in the world knows about the camera: the
	// projection simply shows the rectangle that starts at the camera, so
	// moving the camera is just moving that window over the world
	glViewport(0, HUD_HEIGHT * PIXEL_SCALE, SCREEN_WIDTH, WORLD_VIEW_HEIGHT * PIXEL_SCALE);
	projection = glm::ortho(camera.x, camera.x + CAMERA_WIDTH, camera.y + CAMERA_HEIGHT, camera.y);

	texProgram->use();
	texProgram->setUniformMatrix4f("projection", projection);
	texProgram->setUniform4f("color", 1.0f, 1.0f, 1.0f, 1.0f);
	texProgram->setUniformMatrix4f("modelview", modelview);
	// The map's vertices already have their final positions and texture
	// coordinates, so modelview and texCoordDispl have to be neutral for it
	texProgram->setUniform2f("texCoordDispl", 0.f, 0.f);
	map->render();

	// A closed door is drawn darker: the same tile again on top, tinted grey
	if(hasDoor && !doorOpen())
	{
		texProgram->setUniform4f("color", 0.35f, 0.35f, 0.45f, 1.0f);
		doorSprite->render();
		texProgram->setUniform4f("color", 1.0f, 1.0f, 1.0f, 1.0f);
	}

	// What's drawn later appears on top: items and bombs behind the characters,
	// and the fire over everything
	rayHazards.render(*map, levelMs);

	bombSystem.renderItemsAndBombs(*texProgram);
	if(boss)
		boss->render();
	for(auto &enemy : enemies)
		enemy->render();
	player->render();
	for(auto &projectile : projectiles)
		projectile->render();
	bombSystem.renderExplosions(*texProgram);

	// Messages over the world, in screen coordinates (without the camera, so
	// they stay still while the world scrolls)
	projection = glm::ortho(0.f, float(VIEW_WIDTH), float(WORLD_VIEW_HEIGHT), 0.f);
	texProgram->setUniformMatrix4f("projection", projection);

	stringstream stage;
	stage << "STAGE " << currentLevel;
	switch(state)
	{
	case State::PLAYING:
		if(levelMs < STAGE_TITLE_MS)
			renderBanner(stage.str(), currentLevel == NUM_LEVELS ? "DEFEAT THE BOSS!" : "DEFEAT ALL THE ENEMIES!");
		else if(godMode)
			bigText->render("GOD MODE", glm::vec2(4.f, 4.f), glm::vec4(1.f, 1.f, 0.1f, 1.f));
		break;
	case State::LEVEL_CLEAR:
		renderBanner(stage.str() + " CLEAR!", "");
		break;
	case State::PLAYER_DEAD:
		if(timeUp)
			// No subtitle: with lives left the level just starts again, and if it
			// was the last one the GAME OVER banner comes right after
			renderBanner("TIME UP!", "");
		break;
	case State::GAME_OVER:
		renderBanner("GAME OVER", "PRESS ENTER TO GO BACK TO THE MENU");
		break;
	case State::VICTORY:
		renderBanner("CONGRATULATIONS!", "YOU BEAT THE GAME!");
		break;
	}

	// HUD bar: its own viewport at the bottom, pink background and the info on top
	glViewport(0, 0, SCREEN_WIDTH, HUD_HEIGHT * PIXEL_SCALE);
	game.fillRect(glm::vec4(0.f, float(VIEW_HEIGHT - HUD_HEIGHT), float(VIEW_WIDTH), float(HUD_HEIGHT)),
		glm::vec3(0.95f, 0.45f, 0.7f));
	projection = glm::ortho(0.f, float(VIEW_WIDTH), float(HUD_HEIGHT), 0.f);
	texProgram->setUniformMatrix4f("projection", projection);
	renderHUD();

	// Leave the full screen viewport for whatever is drawn next
	glViewport(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT);
}

// A dark band across the middle of the screen with a title and an optional subtitle
void Scene::renderBanner(const string &title, const string &subtitle)
{
	float height = subtitle.empty() ? 28.f : 40.f;
	float y = (WORLD_VIEW_HEIGHT - height) / 2.f;

	// The band is painted with Game::fillRect (a scissor clear). The world
	// viewport starts at the top of the window, so its coordinates match the
	// 320x240 screen ones and the band lines up with the text
	Game::instance().fillRect(glm::vec4(0.f, y, float(VIEW_WIDTH), height), BANNER_BACKGROUND_COLOR);

	bigText->renderCentered(title, VIEW_WIDTH / 2.f, y + 6.f, BANNER_TITLE_COLOR);
	if(!subtitle.empty())
		text->renderCentered(subtitle, VIEW_WIDTH / 2.f, y + 26.f, BANNER_SUBTITLE_COLOR);
}

// The bar at the bottom: time, lives, bombs, fire, speed, heart, enemies left
// and the stage (or the boss health)
void Scene::renderHUD()
{
	const PlayerStats &stats = player->getStats();
	const glm::vec4 dark(0.1f, 0.05f, 0.1f, 1.f);
	const float textY = 4.f;

	// Same 8x8 sprite for every icon: we just change its frame and position
	auto drawSmallIcon = [&](const glm::ivec2 &pixel, float x)
	{
		smallIcon->setFrame(atlasPixel(pixel.x, pixel.y));
		smallIcon->setPosition(glm::vec2(x, textY));
		smallIcon->render();
	};
	auto number = [](int value)
	{
		stringstream result;
		result << value;
		return result.str();
	};

	// Remaining time (blinking red in the last 30 s)
	drawSmallIcon(ICON_TIME, 4.f);
	glm::vec4 timeColor = timeLeftMs < HURRY_UP_MS && (timeLeftMs / 250) % 2 == 0 ? glm::vec4(0.8f, 0.f, 0.f, 1.f) : dark;
	text->render(formatTime(timeLeftMs), glm::vec2(14.f, textY), timeColor);

	// Lives
	drawSmallIcon(ICON_LIVES, 50.f);
	text->render(number(lives), glm::vec2(60.f, textY), dark);

	// Simultaneous bombs and explosion size
	drawSmallIcon(ICON_BOMB, 76.f);
	text->render(number(stats.maxBombs), glm::vec2(86.f, textY), dark);
	drawSmallIcon(ICON_FIRE, 102.f);
	text->render(number(stats.fireRange), glm::vec2(112.f, textY), dark);

	// Speed and heart item
	drawSmallIcon(ICON_SPEED, 128.f);
	text->render(number(stats.speedLevel), glm::vec2(138.f, textY), dark);
	if(stats.heart)
		drawSmallIcon(ICON_HEART, 154.f);

	// Enemies left
	icon->setFrame(atlasCell(0, 5));
	icon->setPosition(glm::vec2(166.f, 0.f));
	icon->render();
	text->render("X" + number(enemiesLeft()), glm::vec2(184.f, textY), dark);

	// Boss health
	if(boss && !boss->isDefeated())
	{
		// One '=' per health point
		string health(boss->getHealth(), '=');
		text->render("BOSS " + health, glm::vec2(212.f, textY), glm::vec4(0.6f, 0.f, 0.f, 1.f));
	}
	else
	{
		stringstream stage;
		stage << "STAGE " << currentLevel;
		text->render(stage.str(), glm::vec2(VIEW_WIDTH - 4.f - text->width(stage.str()), textY), dark);
	}
}
