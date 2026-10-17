#ifndef _MENU_INCLUDE
#define _MENU_INCLUDE

#include <memory>

#include "Sprite.h"
#include "Text.h"

// Screens outside the game itself: main menu, instructions and credits
class Menu
{
public:
    enum class Action
    {
        NONE,
        PLAY,
        QUIT
    };

    void init(ShaderProgram &program, Texture &atlas, Text &text, Text &bigText);
    void shutdown();

    // Go back to the main screen
    void reset();
    void update(int deltaTime);
    void render();
    Action keyPressed(int key);

private:
    enum class Screen
    {
        MAIN,
        INSTRUCTIONS,
        CREDITS
    };

    void renderMain();
    void renderInstructions();
    void renderCredits();
    void renderIcon(int column, int row, const glm::vec2 &pos);

private:
    ShaderProgram *program = nullptr;
    Text *text = nullptr;
    Text *bigText = nullptr;

    std::unique_ptr<Sprite> bomberman;
    std::unique_ptr<Sprite> icon;

    Screen screen = Screen::MAIN;
    int selected = 0;
    int elapsedMs = 0;
};

#endif // _MENU_INCLUDE
