#include <cmath>

#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <glm/gtc/matrix_transform.hpp>

#include "Menu.h"
#include "Game.h"
#include "Gameplay.h"
#include "Audio.h"

namespace
{
    const char *OPTIONS[] = { "PLAY", "INSTRUCTIONS", "CREDITS", "EXIT" };
    constexpr int OPTION_COUNT = 4;

    const glm::vec4 TITLE_COLOR(1.f, 0.85f, 0.2f, 1.f);
    const glm::vec4 HIGHLIGHT_COLOR(1.f, 1.f, 1.f, 1.f);
    const glm::vec4 NORMAL_COLOR(0.6f, 0.65f, 0.9f, 1.f);
    const glm::vec4 HEADER_COLOR(1.f, 0.5f, 0.7f, 1.f);
}

void Menu::init(ShaderProgram &shaderProgram, Texture &atlas, Text &smallText, Text &largeText)
{
    program = &shaderProgram;
    text = &smallText;
    bigText = &largeText;

    // Bomberman walking in the main menu, drawn twice as big
    bomberman.reset(Sprite::createSprite(glm::vec2(32.f), atlasCellSize(), &atlas, program));
    bomberman->setNumberAnimations(1);
    bomberman->setAnimationSpeed(0, 8);
    for (int column : { 0, 1, 0, 2 })
        bomberman->addKeyframe(0, atlasCell(column, 0));
    bomberman->changeAnimation(0);
    // The atlas frames face left; the menu character walks to the right.
    bomberman->setFlipX(true);

    icon.reset(Sprite::createSprite(glm::vec2(16.f), atlasCellSize(), &atlas, program));
}

void Menu::shutdown()
{
    bomberman.reset();
    icon.reset();
}

void Menu::reset()
{
    screen = Screen::MAIN;
    selected = 0;
    Audio::instance().playMusic(Music::TITLE);
}

void Menu::update(int deltaTime)
{
    elapsedMs += deltaTime;
    if (bomberman)
        bomberman->update(deltaTime);
}

Menu::Action Menu::keyPressed(int key)
{
    if (screen != Screen::MAIN)
    {
        // Any of these keys go back to the main screen
        if (key == GLFW_KEY_ESCAPE || key == GLFW_KEY_ENTER || key == GLFW_KEY_SPACE)
        {
            screen = Screen::MAIN;
            Audio::instance().playSound(Sound::MENU_SELECT);
            Audio::instance().playMusic(Music::TITLE);
        }
        return Action::NONE;
    }

    switch (key)
    {
    case GLFW_KEY_UP:
        selected = (selected + OPTION_COUNT - 1) % OPTION_COUNT;
        Audio::instance().playSound(Sound::MENU_MOVE);
        break;
    case GLFW_KEY_DOWN:
        selected = (selected + 1) % OPTION_COUNT;
        Audio::instance().playSound(Sound::MENU_MOVE);
        break;
    case GLFW_KEY_ESCAPE:
        return Action::QUIT;
    case GLFW_KEY_ENTER:
    case GLFW_KEY_SPACE:
        Audio::instance().playSound(Sound::MENU_SELECT);
        switch (selected)
        {
        case 0:
            return Action::PLAY;
        case 1:
            screen = Screen::INSTRUCTIONS;
            Audio::instance().playMusic(Music::MAP);
            break;
        case 2:
            screen = Screen::CREDITS;
            Audio::instance().playMusic(Music::MAP);
            break;
        default:
            return Action::QUIT;
        }
        break;
    }

    return Action::NONE;
}

void Menu::render()
{
    glm::mat4 projection = glm::ortho(0.f, float(VIEW_WIDTH), float(VIEW_HEIGHT), 0.f);
    glm::mat4 modelview(1.f);

    Game::instance().fillRect(glm::vec4(0.f, 0.f, float(VIEW_WIDTH), float(VIEW_HEIGHT)),
                              glm::vec3(0.12f, 0.12f, 0.35f));

    program->use();
    program->setUniformMatrix4f("projection", projection);
    program->setUniformMatrix4f("modelview", modelview);
    program->setUniform4f("color", 1.f, 1.f, 1.f, 1.f);
    program->setUniform2f("texCoordDispl", 0.f, 0.f);

    switch (screen)
    {
    case Screen::MAIN:
        renderMain();
        break;
    case Screen::INSTRUCTIONS:
        renderInstructions();
        break;
    case Screen::CREDITS:
        renderCredits();
        break;
    }
}

void Menu::renderIcon(int column, int row, const glm::vec2 &pos)
{
    icon->setFrame(atlasCell(column, row));
    icon->setPosition(pos);
    icon->render();
}

void Menu::renderMain()
{
    // Title bouncing a little
    const float bounce = std::round(std::sin(elapsedMs / 300.f) * 2.f);
    text->renderCentered("POCKET", VIEW_WIDTH / 2.f, 30.f + bounce, TITLE_COLOR);
    bigText->renderCentered("BOMBERMAN", VIEW_WIDTH / 2.f, 42.f + bounce, TITLE_COLOR);

    // Bomberman walks across the screen
    const float walkX = std::fmod(elapsedMs / 25.f, VIEW_WIDTH + 64.f) - 32.f;
    bomberman->setPosition(glm::vec2(std::round(walkX), 76.f));
    bomberman->render();

    for (int i = 0; i < OPTION_COUNT; ++i)
    {
        const float y = 130.f + i * 18.f;
        const bool isSelected = (i == selected);
        text->renderCentered(OPTIONS[i], VIEW_WIDTH / 2.f, y, isSelected ? HIGHLIGHT_COLOR : NORMAL_COLOR);

        // A bomb points at the selected option
        if (isSelected)
            renderIcon(0, 2, glm::vec2(VIEW_WIDTH / 2.f - text->width(OPTIONS[i]) / 2.f - 22.f, y - 4.f));
    }

    text->renderCentered("UP/DOWN + ENTER", VIEW_WIDTH / 2.f, 220.f, NORMAL_COLOR);
}

void Menu::renderInstructions()
{
    bigText->renderCentered("INSTRUCTIONS", VIEW_WIDTH / 2.f, 8.f, TITLE_COLOR);

    float y = 32.f;
    auto line = [&](const char *str, const glm::vec4 &color)
    {
        text->render(str, glm::vec2(12.f, y), color);
        y += 11.f;
    };

    line("DEFEAT ALL THE ENEMIES AND REACH THE DOOR", HIGHLIGHT_COLOR);
    line("BEFORE THE TIME RUNS OUT. BOMBS FLOAT IN THE", HIGHLIGHT_COLOR);
    line("AIR: JUMP ON THEM TO REACH HIGH PLACES!", HIGHLIGHT_COLOR);
    y += 4.f;
    line("LEFT/RIGHT   MOVE", NORMAL_COLOR);
    line("UP OR Z      JUMP", NORMAL_COLOR);
    line("SPACE OR X   DROP A BOMB", NORMAL_COLOR);
    line("ESC          BACK TO THE MENU", NORMAL_COLOR);

    // Items with their icons
    y += 6.f;
    const char *names[] = { "BOMB UP: +1 BOMB", "FIRE UP: +1 FIRE", "HEART: SURVIVE A HIT", "SPEED UP: RUN FASTER" };
    for (int i = 0; i < 4; ++i)
    {
        const glm::vec2 pos(12.f + (i % 2) * 152.f, y + (i / 2) * 20.f);
        renderIcon(i, 4, pos);
        text->render(names[i], pos + glm::vec2(20.f, 4.f), NORMAL_COLOR);
    }
    y += 46.f;

    line("CHEATS:", HEADER_COLOR);
    line("G GOD MODE   K KILL ENEMIES   1-5 GO TO LEVEL", NORMAL_COLOR);
}

void Menu::renderCredits()
{
    bigText->renderCentered("CREDITS", VIEW_WIDTH / 2.f, 16.f, TITLE_COLOR);

    text->renderCentered("PROGRAMMING", VIEW_WIDTH / 2.f, 56.f, HEADER_COLOR);
    text->renderCentered("JUN ZHE XIA", VIEW_WIDTH / 2.f, 70.f, HIGHLIGHT_COLOR);
    text->renderCentered("DAIWEI CHEN", VIEW_WIDTH / 2.f, 82.f, HIGHLIGHT_COLOR);

    text->renderCentered("BASED ON", VIEW_WIDTH / 2.f, 112.f, HEADER_COLOR);
    text->renderCentered("POCKET BOMBERMAN (HUDSON SOFT, 1998)", VIEW_WIDTH / 2.f, 126.f, HIGHLIGHT_COLOR);

    text->renderCentered("VIDEOJOCS - FIB - UPC", VIEW_WIDTH / 2.f, 160.f, NORMAL_COLOR);
    text->renderCentered("CURS 2026/27", VIEW_WIDTH / 2.f, 172.f, NORMAL_COLOR);

    text->renderCentered("PRESS ENTER TO GO BACK", VIEW_WIDTH / 2.f, 220.f, NORMAL_COLOR);
}
