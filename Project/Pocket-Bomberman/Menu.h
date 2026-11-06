#ifndef _MENU_INCLUDE
#define _MENU_INCLUDE

#include <memory>

#include "Sprite.h"
#include "Text.h"

// Screens outside the game itself: main menu, instructions and credits.
// The menu doesn't start the game or close the window by itself: keyPressed
// returns what the player chose, and Game acts on it.
class Menu
{
public:
    // What the player chose this key press
    enum class Action
    {
        NONE,
        PLAY,
        QUIT
    };

    void init(ShaderProgram &program, Texture &atlas, Text &text, Text &bigText);
    void shutdown();

    // Go back to the main screen (and the title music)
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

    // The first picture of the intro, used as a dim background of the main screen
    std::unique_ptr<Texture> backgroundTexture;
    std::unique_ptr<Sprite> background;
    // Bomberman walking across the main screen
    std::unique_ptr<Sprite> bomberman;
    // Reused for every icon (the bomb cursor and the items of the instructions)
    std::unique_ptr<Sprite> icon;

    Screen screen = Screen::MAIN;
    // Which option the cursor is on
    int selected = 0;
    // Time since the start, for the small animations of the main screen
    int elapsedMs = 0;
};

#endif // _MENU_INCLUDE
