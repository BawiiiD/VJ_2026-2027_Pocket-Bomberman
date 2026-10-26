#include <iostream>
#include <algorithm>
#include "Game.h"
#include "Audio.h"


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
	// Don't repeat the texture at the edges
	atlas.setWrapS(GL_CLAMP_TO_EDGE);
	atlas.setWrapT(GL_CLAMP_TO_EDGE);
	// For pixel art
	atlas.setMinFilter(GL_NEAREST);
	atlas.setMagFilter(GL_NEAREST);
	// Same font, different sizes
	text.init(texProgram, 8);
	bigText.init(texProgram, 16);

	Audio::instance().init();

	menu.init(texProgram, atlas, text, bigText);
	scene.init(texProgram, atlas, text, bigText);

	// Starts the menu music
	menu.reset();
	state = GameState::MENU;
}

bool Game::update(int deltaTime)
{
	// Avoid huge steps (for example while the window is being dragged)
	deltaTime = std::min(deltaTime, 50);

	switch(state)
	{
	case GameState::MENU:
		menu.update(deltaTime);
		break;
	case GameState::PLAYING:
		scene.update(deltaTime);
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
	// Reset the viewport every frame
	glViewport(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	if(state == GameState::MENU)
		menu.render();
	else
		scene.render();
}

void Game::keyPressed(int key)
{
	if(key < 0 || key > GLFW_KEY_LAST)
		return;
	keys[key] = true;

	if(state == GameState::MENU)
	{
		switch(menu.keyPressed(key))
		{
		case Menu::Action::PLAY:
			scene.startGame();
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
	// Scissor coordinates are in window pixels, with the origin at the bottom-left corner
	glEnable(GL_SCISSOR_TEST);
	glScissor(int(rect.x * PIXEL_SCALE),
	          int((VIEW_HEIGHT - rect.y - rect.w) * PIXEL_SCALE),
	          int(rect.z * PIXEL_SCALE),
	          int(rect.w * PIXEL_SCALE));
	glClearColor(color.r, color.g, color.b, 1.f);
	glClear(GL_COLOR_BUFFER_BIT);
	glClearColor(0.f, 0.f, 0.f, 1.f);
	glDisable(GL_SCISSOR_TEST);
}

void Game::shutdown()
{
	// OpenGL objects have to be released while the context still exists
	scene.shutdown();
	menu.shutdown();
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
	vShader.free();
	fShader.free();
}
