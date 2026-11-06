// Entry point: opens the window, connects GLFW's input events to the Game
// and runs the game loop at 60 frames per second.

// NOMINMAX stops windows.h from defining min/max macros, which would break std::min/std::max
#define NOMINMAX
#include <windows.h>
#include <iostream>
#include <string>
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include "Game.h"


#define TARGET_FRAMERATE 60.0f

// GLFW calls these functions when something happens in the window. All they
// do is forward the event to the Game, which decides what to do with it.

void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
	if (action == GLFW_PRESS)
		Game::instance().keyPressed(key);
	else if (action == GLFW_RELEASE)
		Game::instance().keyReleased(key);
}

void cursor_position_callback(GLFWwindow* window, double xpos, double ypos)
{
	Game::instance().mouseMove(int(xpos), int(ypos));
}

void mouse_button_callback(GLFWwindow* window, int button, int action, int mods)
{
	if (action == GLFW_PRESS)
		Game::instance().mousePress(button);
	else if (action == GLFW_RELEASE)
		Game::instance().mouseRelease(button);
}


int main(void)
{
	// The game loads everything with relative paths ("images/...", "levels/..."), so
	// first we move the working directory to the folder that has the assets.
	// The packaged build (Binary folder) has them next to the exe. When we run it
	// from Visual Studio the exe is in Debug/ or Release/, and the assets are in
	// the Pocket-Bomberman folder next to it, so we go there instead.
	wchar_t executablePath[MAX_PATH] = {};
	if(!GetModuleFileNameW(NULL, executablePath, MAX_PATH))
		return -1;
	const std::wstring path(executablePath);
	const std::wstring executableDirectory = path.substr(0, path.find_last_of(L"/\\") + 1);
	std::wstring assetDirectory = executableDirectory;
	// If the .sln is one folder up, we are running a development build
	const bool developmentBuild = GetFileAttributesW((executableDirectory + L"..\\Pocket-Bomberman.sln").c_str()) != INVALID_FILE_ATTRIBUTES;
	if(developmentBuild || GetFileAttributesW((executableDirectory + L"images\\sprites.png").c_str()) == INVALID_FILE_ATTRIBUTES)
		assetDirectory += L"..\\Pocket-Bomberman";
	if(!SetCurrentDirectoryW(assetDirectory.c_str()))
	{
		std::cerr << "Could not find the Pocket-Bomberman asset directory" << std::endl;
		return -1;
	}

	GLFWwindow* window;
	double timePerFrame = 1.f / TARGET_FRAMERATE, timePreviousFrame, currentTime;

	if (!glfwInit())
		return -1;

	// The game is drawn for a fixed resolution, so the window can't be resized
	glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
	window = glfwCreateWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "Pocket Bomberman", NULL, NULL);
	if (!window)
	{
		glfwTerminate();
		return -1;
	}

	glfwSetWindowPos(window, 100, 100);
	// From now on every OpenGL call draws in this window
	glfwMakeContextCurrent(window);

	glfwSetKeyCallback(window, key_callback);
	glfwSetCursorPosCallback(window, cursor_position_callback);
	glfwSetMouseButtonCallback(window, mouse_button_callback);

	// GLEW loads the modern OpenGL functions (shaders, VAOs...). It needs the
	// context to exist, that's why it goes after creating the window
	glewExperimental = GL_TRUE;
	glewInit();

	Game::instance().init();
	timePreviousFrame = glfwGetTime();

	while (!glfwWindowShouldClose(window))
	{
		currentTime = glfwGetTime();
		// We only do a frame when 1/60 s has passed. The rest of the time we just
		// keep reading events, so the game runs at the same speed on every PC
		if (currentTime - timePreviousFrame >= timePerFrame)
		{
			// update() gets the time since the last frame in milliseconds, and
			// returns false when the player chose EXIT
			if(!Game::instance().update(int(1000.0f * (currentTime - timePreviousFrame))))
				glfwSetWindowShouldClose(window, GLFW_TRUE);
			Game::instance().render();
			timePreviousFrame = currentTime;

			// We draw on a hidden buffer and show it all at once, so there's no flickering
			glfwSwapBuffers(window);
		}

		glfwPollEvents();
	}

	// The OpenGL objects have to be freed before the window (and its context) is destroyed
	Game::instance().shutdown();
    glfwDestroyWindow(window);

	glfwTerminate();
	return 0;
}
