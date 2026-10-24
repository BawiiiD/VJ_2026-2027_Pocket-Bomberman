#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include "Game.h"


#define TARGET_FRAMERATE 60.0f

// Update keys[] and mouse position when the user presses/releases a key or moves the mouse
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
	GLFWwindow* window;
	double timePerFrame = 1.f / TARGET_FRAMERATE, timePreviousFrame, currentTime;

	// Initialize the library
	if (!glfwInit())
		return -1;

	// Make the window not resizable
	glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
	// Create a 640x480 windowed mode window and its OpenGL context
	window = glfwCreateWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "Pocket Bomberman", NULL, NULL);
	if (!window)
	{
		glfwTerminate();
		return -1;
	}

	// Set window initial position
	glfwSetWindowPos(window, 100, 100);
	// Make the window's context current
	glfwMakeContextCurrent(window);

	// Set callbacks
	glfwSetKeyCallback(window, key_callback);
	glfwSetCursorPosCallback(window, cursor_position_callback);
	glfwSetMouseButtonCallback(window, mouse_button_callback);

	// Init glew to have access to GL extensions
	glewExperimental = GL_TRUE;
	glewInit();

	// Load resources and initialize the game
	Game::instance().init();
	// Start the clock
	timePreviousFrame = glfwGetTime();

	// Run until user closes the window
	while (!glfwWindowShouldClose(window))
	{
		currentTime = glfwGetTime();
		if (currentTime - timePreviousFrame >= timePerFrame)
		{
			// Update & render steps of the game loop
			if(!Game::instance().update(int(1000.0f * (currentTime - timePreviousFrame))))
				glfwSetWindowShouldClose(window, GLFW_TRUE);
			Game::instance().render();
			timePreviousFrame = currentTime;

			// Swap front and back buffers
			glfwSwapBuffers(window);
		}

		// Process pending window events
		glfwPollEvents();
	}

	Game::instance().shutdown();
    glfwDestroyWindow(window);
	glfwTerminate();
	return 0;
}

