#ifndef _SCENE_INCLUDE
#define _SCENE_INCLUDE


#include <memory>
#include <random>
#include <vector>
#include <glm/glm.hpp>
#include "ShaderProgram.h"
#include "TileMap.h"
#include "Player.h"
#include "Bomb.h"
#include "Explosion.h"
#include "Item.h"
#include "Enemy.h"
#include "Boss.h"
#include "Text.h"


// Scene contains all the entities of the level being played.
// It is responsible for updating and render them, and for the rules of the
// game: lives, time limit, exit door and changing levels.


class Scene
{

public:
	Scene();
	~Scene();

	void init(ShaderProgram &program, Texture &atlas, Text &text, Text &bigText);
	void shutdown();

	// Starts a new game from the first level
	void startGame();
	void update(int deltaTime);
	void render();
	void keyPressed(int key);

	// The game is over (lost or won) and we have to go back to the menu
	bool isFinished() const;

private:
	enum class State
	{
		PLAYING,
		LEVEL_CLEAR,
		PLAYER_DEAD,
		GAME_OVER,
		VICTORY
	};

	bool loadLevel(int level);
	void unloadLevel();

	void placeBomb();
	void updateBombs(int deltaTime);
	void explode(const Bomb &bomb);
	void spawnItem(const glm::ivec2 &tile);
	void updateCollisions();
	void updateCamera();
	void updateDoor();
	bool rayOn() const;
	bool insideRay(const glm::ivec2 &tile) const;
	void killAllEnemies();
	int enemiesLeft() const;
	bool doorOpen() const;
	void changeState(State newState);

	void renderHUD();
	void renderBanner(const string &title, const string &subtitle);

private:
	ShaderProgram *texProgram;
	Texture *atlas;
	Texture explosionTexture;
	Text *text;
	Text *bigText;

	std::unique_ptr<TileMap> map;
	std::unique_ptr<Player> player;
	std::vector<std::unique_ptr<Bomb>> bombs;
	std::vector<std::unique_ptr<Explosion>> explosions;
	std::vector<std::unique_ptr<Item>> items;
	std::vector<std::unique_ptr<Enemy>> enemies;
	std::unique_ptr<Boss> boss;
	std::vector<std::unique_ptr<Projectile>> projectiles;

	// Exit door
	bool hasDoor;
	glm::ivec2 doorTile;
	// Boss level: the door appears here when the boss is defeated
	glm::ivec2 bossDoorTile;
	std::unique_ptr<Sprite> doorSprite;

	// Rays between emitter blocks, all of them shoot at the same time
	std::vector<RayInfo> rays;
	std::unique_ptr<Sprite> raySprite;

	// HUD icons
	std::unique_ptr<Sprite> smallIcon;
	std::unique_ptr<Sprite> icon;

	// Game progress
	int currentLevel;
	int lives;
	PlayerStats levelStartStats;
	bool godMode;

	// Level progress
	State state;
	int stateMs;
	int levelMs;
	int timeLeftMs;
	bool timeUp;
	bool finished;

	// To play the door and time sounds only once
	bool doorWasOpen;
	bool timeWarned;

	glm::vec2 camera;
	std::mt19937 random;

};


#endif // _SCENE_INCLUDE
