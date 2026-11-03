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

	// Below this time the clock blinks and a warning sounds
	constexpr int HURRY_UP_MS = 30000;

	// 8x8 HUD icons in images/sprites.png (pixel coordinates)
	const glm::ivec2 ICON_LIVES(0, 16);
	const glm::ivec2 ICON_BOMB(8, 16);
	const glm::ivec2 ICON_HEART(16, 16);
	const glm::ivec2 ICON_TIME(8, 24);
	const glm::ivec2 ICON_FIRE(0, 48);

	// Contents of a levels/levelXX.txt file
	struct LevelInfo
	{
		string mapFile;
		int time = 180;
		glm::ivec2 player = glm::ivec2(1);
		vector<pair<Enemy::Type, glm::ivec2>> enemies;
		bool hasBoss = false;
		glm::ivec2 boss = glm::ivec2(0);
		glm::ivec2 door = glm::ivec2(0);
	};

	bool loadLevelInfo(const string &fileName, LevelInfo &info)
	{
		ifstream fin(fileName.c_str());
		if(!fin.is_open())
			return false;

		string line;
		while(getline(fin, line))
		{
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

	Music levelMusic(int level, bool hasBoss)
	{
		if(hasBoss)
			return Music::BOSS;

		const Music stages[] = { Music::STAGE1, Music::STAGE2, Music::STAGE3, Music::STAGE4 };
		return stages[glm::clamp(level, 1, 4) - 1];
	}

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

	smallIcon.reset(Sprite::createSprite(glm::vec2(8.f), glm::vec2(8.f / ATLAS_SIZE), atlas, texProgram));
	icon.reset(Sprite::createSprite(glm::vec2(16.f), atlasCellSize(), atlas, texProgram));
}

void Scene::shutdown()
{
	unloadLevel();
	smallIcon.reset();
	icon.reset();
}

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

	unloadLevel();
	currentLevel = level;

	map.reset(TileMap::createTileMap(info.mapFile, *texProgram));
	if(!map->isLoaded())
	{
		finished = true;
		return false;
	}

	player.reset(new Player());
	player->init(*atlas, *texProgram);
	player->setTileMap(map.get());
	player->setTile(info.player);
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

	hasDoor = map->findDoor(doorTile);
	bossDoorTile = info.door;
	doorSprite.reset(Sprite::createSprite(
		glm::vec2(float(map->getTileSize())),
		map->getTileTexSize(),
		&map->getTilesheet(),
		texProgram
	));
	doorSprite->setFrame(map->getTileTexCoord(map->getDoorTileId()));
	updateDoor();

	rayHazards.init(*map, *texProgram);

	timeLeftMs = info.time * 1000;
	timeUp = false;
	timeWarned = timeLeftMs <= HURRY_UP_MS;
	doorWasOpen = hasDoor && doorOpen();
	levelMs = 0;
	changeState(State::PLAYING);
	updateCamera();

	// After dying the music was stopped, so it starts again from the beginning
	Audio::instance().playMusic(levelMusic(level, info.hasBoss));

	return true;
}

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

void Scene::changeState(State newState)
{
	state = newState;
	stateMs = 0;
}

void Scene::keyPressed(int key)
{
	if(!map || finished)
		return;

	// Skip the end messages
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
		// Jump to a level keeping the current lives and items
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
	if(!map || finished)
		return;

	stateMs += deltaTime;
	levelMs += deltaTime;
	map->update(deltaTime);

	glm::vec2 playerCenter = player->getCenter();

	player->update(deltaTime);
	for(auto &enemy : enemies)
		enemy->update(deltaTime, playerCenter);
	if(boss)
		boss->update(deltaTime, playerCenter, projectiles);
	for(auto &projectile : projectiles)
		projectile->update(deltaTime, *map);
	bombSystem.update(deltaTime, *map, rayHazards, levelMs, *atlas, *texProgram);

	collisionSystem.update(state == State::PLAYING, *player, enemies, boss.get(), projectiles,
	                       bombSystem, rayHazards, map->getTileSize(), levelMs);

	// Exit door: the player has to be inside the door tile
	if(state == State::PLAYING && !player->isDying() && hasDoor && doorOpen() &&
	   map->toTile(player->getCenter()) == doorTile)
	{
		player->celebrate();
		changeState(State::LEVEL_CLEAR);
		Audio::instance().playMusic(Music::CLEAR, false);
	}
	// Remove the entities that have finished
	enemies.erase(remove_if(enemies.begin(), enemies.end(),
		[](const std::unique_ptr<Enemy> &enemy) { return enemy->isDead(); }), enemies.end());
	projectiles.erase(remove_if(projectiles.begin(), projectiles.end(),
		[](const std::unique_ptr<Projectile> &projectile) { return !projectile->isAlive(); }), projectiles.end());
	bombSystem.removeFinishedExplosions();

	updateDoor();

	switch(state)
	{
	case State::PLAYING:
		timeLeftMs -= deltaTime;
		if(timeLeftMs <= 0)
		{
			timeLeftMs = 0;
			timeUp = true;
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
			if(!timeUp)
				Audio::instance().stopMusic();
			changeState(State::PLAYER_DEAD);
		}
		break;

	case State::LEVEL_CLEAR:
		if(stateMs >= LEVEL_CLEAR_MS)
		{
			levelStartStats = player->getStats();
			if(currentLevel == NUM_LEVELS)
			{
				changeState(State::VICTORY);
				Audio::instance().playMusic(Music::VICTORY, false);
			}
			else
				loadLevel(currentLevel + 1);
		}
		break;

	case State::PLAYER_DEAD:
		if(!player->isAlive() && stateMs >= (timeUp ? TIME_UP_MS : AFTER_DEATH_MS))
		{
			--lives;
			if(lives <= 0)
			{
				changeState(State::GAME_OVER);
				Audio::instance().playMusic(Music::GAME_OVER, false);
			}
			else
				loadLevel(currentLevel);
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

	if(doorSprite)
		doorSprite->setPosition(glm::vec2(doorTile * map->getTileSize()));

	// Sound when the door opens (all the enemies defeated)
	bool open = hasDoor && doorOpen();
	if(open && !doorWasOpen)
		Audio::instance().playSound(Sound::DOOR_OPEN);
	doorWasOpen = open;
}

void Scene::killAllEnemies()
{
	for(auto &enemy : enemies)
		enemy->kill();
	if(boss)
		boss->kill();
	projectiles.clear();
}

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

bool Scene::doorOpen() const
{
	return enemiesLeft() == 0 && (!boss || boss->isDefeated());
}

void Scene::updateCamera()
{
	glm::vec2 view(CAMERA_WIDTH, CAMERA_HEIGHT);
	glm::vec2 mapSize(map->getPixelSize());
	glm::vec2 target = player->getCenter() - view / 2.f;

	for(int axis = 0; axis < 2; axis++)
	{
		// Small maps are centered, big ones follow the player
		if(mapSize[axis] <= view[axis])
			camera[axis] = -(view[axis] - mapSize[axis]) / 2.f;
		else
			camera[axis] = glm::clamp(target[axis], 0.f, mapSize[axis] - view[axis]);
	}

	// Align to screen pixels so the zoomed camera can move one screen pixel at a time.
	constexpr float screenPixelsPerWorldPixel = CAMERA_ZOOM * PIXEL_SCALE;
	camera = glm::round(camera * screenPixelsPerWorldPixel) / screenPixelsPerWorldPixel;
}

void Scene::render()
{
	if(!map)
		return;

	Game &game = Game::instance();
	glm::mat4 projection, modelview(1.0f);

	// World, drawn above the HUD bar
	glViewport(0, HUD_HEIGHT * PIXEL_SCALE, SCREEN_WIDTH, WORLD_VIEW_HEIGHT * PIXEL_SCALE);
	projection = glm::ortho(camera.x, camera.x + CAMERA_WIDTH, camera.y + CAMERA_HEIGHT, camera.y);

	texProgram->use();
	texProgram->setUniformMatrix4f("projection", projection);
	texProgram->setUniform4f("color", 1.0f, 1.0f, 1.0f, 1.0f);
	texProgram->setUniformMatrix4f("modelview", modelview);
	texProgram->setUniform2f("texCoordDispl", 0.f, 0.f);
	map->render();

	// A closed door is drawn darker
	if(hasDoor && !doorOpen())
	{
		texProgram->setUniform4f("color", 0.35f, 0.35f, 0.45f, 1.0f);
		doorSprite->render();
		texProgram->setUniform4f("color", 1.0f, 1.0f, 1.0f, 1.0f);
	}

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

	// Messages over the world, in screen coordinates
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
			text->render("GOD MODE", glm::vec2(4.f, 4.f), glm::vec4(1.f, 1.f, 0.3f, 1.f));
		break;
	case State::LEVEL_CLEAR:
		renderBanner(stage.str() + " CLEAR!", "");
		break;
	case State::PLAYER_DEAD:
		if(timeUp)
			renderBanner("TIME UP!", "");
		break;
	case State::GAME_OVER:
		renderBanner("GAME OVER", "PRESS ENTER");
		break;
	case State::VICTORY:
		renderBanner("CONGRATULATIONS!", "YOU FOUND THE POWER STONE");
		break;
	}

	// HUD bar
	glViewport(0, 0, SCREEN_WIDTH, HUD_HEIGHT * PIXEL_SCALE);
	game.fillRect(glm::vec4(0.f, float(VIEW_HEIGHT - HUD_HEIGHT), float(VIEW_WIDTH), float(HUD_HEIGHT)),
		glm::vec3(0.95f, 0.45f, 0.7f));
	projection = glm::ortho(0.f, float(VIEW_WIDTH), float(HUD_HEIGHT), 0.f);
	texProgram->setUniformMatrix4f("projection", projection);
	renderHUD();

	glViewport(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT);
}

void Scene::renderBanner(const string &title, const string &subtitle)
{
	float height = subtitle.empty() ? 28.f : 40.f;
	float y = (WORLD_VIEW_HEIGHT - height) / 2.f;

	// The banner is drawn with a scissor clear, in window coordinates
	Game::instance().fillRect(glm::vec4(0.f, y, float(VIEW_WIDTH), height), glm::vec3(0.1f, 0.1f, 0.2f));

	bigText->renderCentered(title, VIEW_WIDTH / 2.f, y + 6.f, glm::vec4(1.f, 0.9f, 0.3f, 1.f));
	if(!subtitle.empty())
		text->renderCentered(subtitle, VIEW_WIDTH / 2.f, y + 26.f);
}

void Scene::renderHUD()
{
	const PlayerStats &stats = player->getStats();
	const glm::vec4 dark(0.1f, 0.05f, 0.1f, 1.f);
	const float textY = 4.f;

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

	// Remaining time (red when it is almost over)
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

	// Heart and speed items
	if(stats.heart)
		drawSmallIcon(ICON_HEART, 128.f);
	if(stats.speedLevel > 0)
		text->render(">" + number(stats.speedLevel), glm::vec2(140.f, textY), dark);

	// Enemies left
	icon->setFrame(atlasCell(0, 5));
	icon->setPosition(glm::vec2(166.f, 0.f));
	icon->render();
	text->render("X" + number(enemiesLeft()), glm::vec2(184.f, textY), dark);

	// Boss health
	if(boss && !boss->isDefeated())
	{
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
