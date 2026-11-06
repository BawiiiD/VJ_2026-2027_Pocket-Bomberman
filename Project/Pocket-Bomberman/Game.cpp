#include <iostream>
#include <algorithm>
#include "Game.h"
#include "Audio.h"


// Leaves everything in a safe state.
// The real setup happens in init().
Game::Game()
    : bPlay(false),
      state(GameState::MENU)
{
	for(int i = 0; i <= GLFW_KEY_LAST; ++i)
		keys[i] = false;
}

void Game::init()
{
	bPlay = true;
	glClearColor(0.f, 0.f, 0.f, 1.0f);
	initShaders();

	// All the characters, items and effects are in this image
	if(!atlas.loadFromFile("images/sprites.png", TEXTURE_PIXEL_FORMAT_RGBA))
		cout << "Could not load images/sprites.png" << endl;
	// If a sprite is drawn right at the edge, don't pick pixels from the other side
	atlas.setWrapS(GL_CLAMP_TO_EDGE);
	atlas.setWrapT(GL_CLAMP_TO_EDGE);
	// NEAREST keeps every pixel as a sharp square instead of blurring it (pixel art)
	atlas.setMinFilter(GL_NEAREST);
	atlas.setMagFilter(GL_NEAREST);
	// Same font, two sizes: normal text and titles
	text.init(texProgram, 8);
	bigText.init(texProgram, 16);

	Audio::instance().init();

	intro.init(texProgram, text);
	menu.init(texProgram, atlas, text, bigText);
	scene.init(texProgram, atlas, text, bigText);

	// Starts the menu music, which also plays during the intro.
	// If the intro couldn't load, we go straight to the menu
	menu.reset();
	state = intro.isFinished() ? GameState::MENU : GameState::INTRO;
}

bool Game::update(int deltaTime)
{
	// If the window is dragged the game freezes for a while, and the next frame
	// would get a huge deltaTime (things would jump through walls). We cap it at 50 ms
	deltaTime = std::min(deltaTime, 50);

	switch(state)
	{
	case GameState::INTRO:
		intro.update(deltaTime);
		if(intro.isFinished())
			state = GameState::MENU;
		break;
	case GameState::MENU:
		menu.update(deltaTime);
		break;
	case GameState::PLAYING:
		scene.update(deltaTime);
		// The scene says it's finished after game over or after the victory screen
		if(scene.isFinished())
		{
			menu.reset();
			state = GameState::MENU;
		}
		break;
	}

	return bPlay;
}

void Game::render()
{
	// The scene changes the viewport to draw the HUD, so we reset it every frame
	glViewport(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	if(state == GameState::INTRO)
		intro.render();
	else if(state == GameState::MENU)
		menu.render();
	else
		scene.render();
}

// keyPressed is called once per press, so it's where we handle the actions
// that happen once (drop a bomb, choose a menu option...). For movement,
// Player asks getKey() every frame instead.
void Game::keyPressed(int key)
{
	if(key < 0 || key > GLFW_KEY_LAST)
		return;
	keys[key] = true;

	// Any of these keys skips the intro
	if(state == GameState::INTRO)
	{
		if(key == GLFW_KEY_ENTER || key == GLFW_KEY_SPACE || key == GLFW_KEY_ESCAPE)
		{
			intro.skip();
			state = GameState::MENU;
		}
		return;
	}

	if(state == GameState::MENU)
	{
		switch(menu.keyPressed(key))
		{
		case Menu::Action::PLAY:
			scene.startGame();
			// startGame() leaves the scene "finished" if the first level failed to load
			if(!scene.isFinished())
				state = GameState::PLAYING;
			break;
		case Menu::Action::QUIT:
			bPlay = false;
			break;
		default:
			break;
		}
	}
	else
	{
		// Escape goes back to the menu
		if(key == GLFW_KEY_ESCAPE)
		{
			menu.reset();
			state = GameState::MENU;
		}
		else
			scene.keyPressed(key);
	}
}

void Game::keyReleased(int key)
{
	if(key >= 0 && key <= GLFW_KEY_LAST)
		keys[key] = false;
}

// The game doesn't use the mouse, but the callbacks are still connected in main.cpp
void Game::mouseMove(int x, int y)
{
}

void Game::mousePress(int button)
{
}

void Game::mouseRelease(int button)
{
}

bool Game::getKey(int key) const
{
	return keys[key];
}

void Game::fillRect(const glm::vec4 &rect, const glm::vec3 &color) const
{
	// Limit the drawing area with the scissor and clear it with the color we want.
	// The scissor works in window pixels with (0,0) at the bottom-left, so we scale and flip the y
	glEnable(GL_SCISSOR_TEST);
	glScissor(int(rect.x * PIXEL_SCALE),
	          int((VIEW_HEIGHT - rect.y - rect.w) * PIXEL_SCALE),
	          int(rect.z * PIXEL_SCALE),
	          int(rect.w * PIXEL_SCALE));
	glClearColor(color.r, color.g, color.b, 1.f);
	glClear(GL_COLOR_BUFFER_BIT);
	// Leave the clear color and the scissor as they were
	glClearColor(0.f, 0.f, 0.f, 1.f);
	glDisable(GL_SCISSOR_TEST);
}

void Game::shutdown()
{
	scene.shutdown();
	menu.shutdown();
	intro.shutdown();
	text.free();
	bigText.free();
	texProgram.free();
	Audio::instance().shutdown();
}

void Game::initShaders()
{
	Shader vShader, fShader;

	vShader.initFromFile(VERTEX_SHADER, "shaders/texture.vert");
	if(!vShader.isCompiled())
	{
		cout << "Vertex Shader Error" << endl;
		cout << "" << vShader.log() << endl << endl;
	}
	fShader.initFromFile(FRAGMENT_SHADER, "shaders/texture.frag");
	if(!fShader.isCompiled())
	{
		cout << "Fragment Shader Error" << endl;
		cout << "" << fShader.log() << endl << endl;
	}
	texProgram.init();
	texProgram.addShader(vShader);
	texProgram.addShader(fShader);
	texProgram.link();
	if(!texProgram.isLinked())
	{
		cout << "Shader Linking Error" << endl;
		cout << "" << texProgram.log() << endl << endl;
	}
	texProgram.bindFragmentOutput("outColor");
	// Once linked into the program, the separate shaders aren't needed anymore
	vShader.free();
	fShader.free();
}
