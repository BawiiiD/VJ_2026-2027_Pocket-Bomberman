#ifndef _SCENE_INCLUDE
#define _SCENE_INCLUDE


#include <memory>
#include <vector>
#include <glm/glm.hpp>
#include "ShaderProgram.h"
#include "TileMap.h"
#include "Player.h"
#include "BombSystem.h"
#include "CollisionSystem.h"
#include "RayHazards.h"
#include "Enemy.h"
#include "Boss.h"
#include "Text.h"


// Scene is the level being played. It owns everything in it (map, player,
// enemies, boss, bombs...) and calls them in the right order every frame.
// It's also the referee of the game: lives, the time limit, when the door
// opens, the cheats, and moving on to the next level or back to the menu.
// The bombs, the collisions and the rays are in their own classes
// (BombSystem, CollisionSystem, RayHazards) to keep this file manageable.


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
	// What's happening in the level. Each state shows its own banner and waits
	// a bit (stateMs) before moving on:
	//   PLAYING -> LEVEL_CLEAR -> next level, or VICTORY after level 5
	//   PLAYING -> PLAYER_DEAD -> same level again, or GAME_OVER with no lives left
	enum class State
	{
		PLAYING,
		LEVEL_CLEAR,
		PLAYER_DEAD,
		GAME_OVER,
		VICTORY
	};

	// Reads levels/levelXX.txt and its map, and creates everything in the level
	bool loadLevel(int level);
	void unloadLevel();

	void updateCamera();
	void updateDoor();
	void killAllEnemies();
	int enemiesLeft() const;
	bool doorOpen() const;
	void changeState(State newState);

	void renderHUD();
	void renderBanner(const string &title, const string &subtitle);

private:
	// Borrowed from Game (Game owns them, we don't delete them)
	ShaderProgram *texProgram;
	Texture *atlas;
	Text *text;
	Text *bigText;

	// unique_ptr means "Scene owns it": when we reset or clear them, they're deleted

	std::unique_ptr<TileMap> map;
	std::unique_ptr<Player> player;
	std::vector<std::unique_ptr<Enemy>> enemies;
	std::unique_ptr<Boss> boss;
	std::vector<std::unique_ptr<Projectile>> projectiles;

	// Exit door
	bool hasDoor;
	glm::ivec2 doorTile;
	// Boss level: the door appears here when the boss is defeated
	glm::ivec2 bossDoorTile;
	std::unique_ptr<Sprite> doorSprite;

	BombSystem bombSystem;
	CollisionSystem collisionSystem;
	RayHazards rayHazards;

	// HUD icons
	std::unique_ptr<Sprite> smallIcon;
	std::unique_ptr<Sprite> icon;

	// Game progress (survives from one level to the next)
	int currentLevel;
	int lives;
	// The player's items when the level started: given back if he dies
	PlayerStats levelStartStats;
	// Remembered here so the G cheat stays on when the level changes
	bool godMode;

	// Level progress
	State state;
	int stateMs;      // time in the current state
	int levelMs;      // time since the level started (title banner, rays)
	int timeLeftMs;   // the clock of the HUD
	bool timeUp;
	// True means Game has to go back to the menu
	bool finished;

	// To play the door and time sounds only once
	bool doorWasOpen;
	bool timeWarned;

	// Top-left corner of the part of the world we see
	glm::vec2 camera;

};


#endif // _SCENE_INCLUDE
