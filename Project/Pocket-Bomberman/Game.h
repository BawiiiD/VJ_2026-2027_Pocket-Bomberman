#ifndef _GAME_INCLUDE
#define _GAME_INCLUDE


#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include "ShaderProgram.h"
#include "Texture.h"
#include "Text.h"
#include "Menu.h"
#include "Intro.h"
#include "Scene.h"


// Size of the window, in real pixels
#define SCREEN_WIDTH 640
#define SCREEN_HEIGHT 480

// The whole game is designed for 320x240, like a small console screen, and
// every one of those pixels is drawn as 2x2 window pixels. This way all the
// positions in the code are in "game pixels" and the pixel art stays sharp.
#define PIXEL_SCALE 2
#define VIEW_WIDTH (SCREEN_WIDTH / PIXEL_SCALE) // 320
#define VIEW_HEIGHT (SCREEN_HEIGHT / PIXEL_SCALE) // 240


// Game is the whole application. It's a singleton (there's only one, and
// anyone can reach it with Game::instance()), because the GLFW callbacks and
// classes like Player need to ask it things, such as which keys are held.
// It owns the things shared by every screen (shader, sprite atlas, fonts)
// and decides which screen is active: the intro, the menu or the game.


class Game
{

private:
	// Private so nobody can create a second Game
	Game();

public:
	static Game &instance()
	{
		// Created the first time someone asks for it, and lives until the program ends
		static Game G;

		return G;
	}

	void init();
	void shutdown();
	// Returns false when the game has to close
	bool update(int deltaTime);
	void render();

	// Called from the GLFW callbacks in main.cpp
	void keyPressed(int key);
	void keyReleased(int key);
	void mouseMove(int x, int y);
	void mousePress(int button);
	void mouseRelease(int button);

	// Is this key held down right now? (for movement, which lasts while the key is held)
	bool getKey(int key) const;

	// Fills a rectangle (in 320x240 screen coordinates) with a solid color.
	// We use it for backgrounds, the HUD bar and the message banners
	void fillRect(const glm::vec4 &rect, const glm::vec3 &color) const;

private:
	enum class GameState
	{
		INTRO,
		MENU,
		PLAYING
	};

	void initShaders();

private:
	bool bPlay; // Becomes false when the player chooses EXIT
	bool keys[GLFW_KEY_LAST+1]; // Which keys are held down, so we can
							    // check them at any moment
	GameState state;

	// Shared resources
	ShaderProgram texProgram;
	Texture atlas;      // images/sprites.png: characters, items and effects
	Text text, bigText; // The same font at two sizes

	Intro intro;
	Menu menu;
	Scene scene;

};


#endif // _GAME_INCLUDE
