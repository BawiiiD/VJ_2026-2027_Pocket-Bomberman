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


#define SCREEN_WIDTH 640
#define SCREEN_HEIGHT 480

// The whole game is designed in a 320x240 resolution,
// and each logical pixel becomes 2x2 window pixels
#define PIXEL_SCALE 2
#define VIEW_WIDTH (SCREEN_WIDTH / PIXEL_SCALE) // 320
#define VIEW_HEIGHT (SCREEN_HEIGHT / PIXEL_SCALE) // 240


// Game is a singleton (a class with a single instance) that represents our whole application.
// It owns the resources shared by all screens and switches between the menu and the game.


class Game
{

private:
	Game();

public:
	static Game &instance()
	{
		static Game G;

		return G;
	}

	void init();
	void shutdown();
	bool update(int deltaTime);
	void render();

	// Input callback methods
	void keyPressed(int key);
	void keyReleased(int key);
	void mouseMove(int x, int y);
	void mousePress(int button);
	void mouseRelease(int button);

	bool getKey(int key) const;

	// Fills a rectangle (in 320x240 screen coordinates) with a solid color
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
	bool bPlay; // Continue to play game?
	bool keys[GLFW_KEY_LAST+1]; // Store key states so that
							    // we can have access at any time
	GameState state;

	// Shared resources
	ShaderProgram texProgram;
	Texture atlas;
	Text text, bigText;

	Intro intro;
	Menu menu;
	Scene scene;

};


#endif // _GAME_INCLUDE
