#include <algorithm>
#include <cmath>
#include <iostream>

#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <glm/gtc/matrix_transform.hpp>

#include "Menu.h"
#include "Game.h"
#include "Gameplay.h"
#include "Audio.h"

namespace
{
    // The options of the main screen, in order (keyPressed uses the same order)
    const char *OPTIONS[] = { "PLAY", "INSTRUCTIONS", "CREDITS", "EXIT" };
    constexpr int OPTION_COUNT = 4;

    const glm::vec3 BACKGROUND_COLOR(0.10f, 0.10f, 0.10f);
    const glm::vec4 TITLE_COLOR(1.f, 0.95f, 0.65f, 1.f);
    const glm::vec4 HIGHLIGHT_COLOR(1.f, 0.95f, 0.65f, 1.f);
    const glm::vec4 MAIN_TEXT_COLOR(1.f, 1.f, 1.f, 1.f);
    const glm::vec4 NORMAL_COLOR(0.82f, 0.83f, 0.85f, 1.f);
    const glm::vec4 HEADER_COLOR(1.f, 0.95f, 0.65f, 1.f);
}

void Menu::init(ShaderProgram &shaderProgram, Texture &atlas, Text &smallText, Text &largeText)
{
    program = &shaderProgram;
    text = &smallText;
    bigText = &largeText;

    background.reset();
    backgroundTexture.reset(new Texture());
    if(backgroundTexture->loadFromFile("images/intro.png", TEXTURE_PIXEL_FORMAT_RGBA))
    {
        backgroundTexture->setWrapS(GL_CLAMP_TO_EDGE);
        backgroundTexture->setWrapT(GL_CLAMP_TO_EDGE);
        backgroundTexture->setMinFilter(GL_NEAREST);
        backgroundTexture->setMagFilter(GL_NEAREST);

        // The first Intro picture occupies x=28..186, y=18..97 in the atlas.
        // We scale it up until it covers the whole screen (keeping its shape)
        // and center it, so some of it is cut at the sides
        const glm::vec2 picturePosition(28.f, 18.f);
        const glm::vec2 pictureSize(159.f, 80.f);
        const glm::vec2 textureSize(float(backgroundTexture->width()), float(backgroundTexture->height()));
        const glm::vec2 screenSize(float(VIEW_WIDTH), float(VIEW_HEIGHT));
        const float scale = std::max(screenSize.x / pictureSize.x, screenSize.y / pictureSize.y);
        background.reset(Sprite::createSprite(pictureSize * scale, pictureSize / textureSize,
                                               backgroundTexture.get(), program));
        background->setFrame(picturePosition / textureSize);
        background->setPosition(glm::round((screenSize - pictureSize * scale) / 2.f));
    }
    else
    {
        backgroundTexture.reset();
        std::cout << "Could not load menu background images/intro.png" << std::endl;
    }

    // Bomberman walking in the main menu, drawn twice as big (same walking frames as in the game)
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

// The three screens share the same key handling: in the main one you move the
// cursor and choose; in the other two any of these keys takes you back

void Menu::shutdown()
{
    background.reset();
    backgroundTexture.reset();
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
    bomberman->update(deltaTime);
}

Menu::Action Menu::keyPressed(int key)
{
    if (screen != Screen::MAIN)
    {
        // Any of these keys go back to the main screen
        if (key == GLFW_KEY_ESCAPE || key == GLFW_KEY_ENTER || key == GLFW_KEY_SPACE || key == GLFW_KEY_Z)
        {
            screen = Screen::MAIN;
            Audio::instance().playSound(Sound::MENU_SELECT);
            Audio::instance().playMusic(Music::TITLE);
        }
        return Action::NONE;
    }

    switch (key)
    {
    // The cursor wraps around: up from the first option goes to the last one.
    // We add OPTION_COUNT before the % so it never becomes negative
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
    case GLFW_KEY_Z:
        Audio::instance().playSound(Sound::MENU_SELECT);
        switch (selected)
        {
        case 0:
            return Action::PLAY;
        // The instructions and the credits have the calmer map music
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

    // Plain background first; the picture (main screen only) goes over it
    Game::instance().fillRect(glm::vec4(0.f, 0.f, float(VIEW_WIDTH), float(VIEW_HEIGHT)),
                              BACKGROUND_COLOR);

    // Menus are drawn in plain 320x240 screen coordinates, without any camera.
    // We reset every uniform because the scene may have left other values
    program->use();
    program->setUniformMatrix4f("projection", projection);
    program->setUniformMatrix4f("modelview", modelview);
    program->setUniform4f("color", 1.f, 1.f, 1.f, 1.f);
    program->setUniform3f("recolor", 0.f, 0.f, 0.f);
    program->setUniform2f("texCoordDispl", 0.f, 0.f);

    if(screen == Screen::MAIN && background)
    {
        // Dim the picture so that the menu text remains readable over it
        // (the shader multiplies the texture by "color")
        program->setUniform4f("color", 0.45f, 0.45f, 0.45f, 1.f);
        background->render();
        program->setUniform4f("color", 1.f, 1.f, 1.f, 1.f);
        program->setUniformMatrix4f("modelview", modelview);
        program->setUniform2f("texCoordDispl", 0.f, 0.f);
    }

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
    // Title bouncing a little: 2 pixels up and down with a sine, rounded to whole pixels
    const float bounce = std::round(std::sin(elapsedMs / 300.f) * 2.f);
    text->renderCentered("POCKET", VIEW_WIDTH / 2.f, 30.f + bounce, TITLE_COLOR);
    bigText->renderCentered("BOMBERMAN", VIEW_WIDTH / 2.f, 42.f + bounce, TITLE_COLOR);

    // Bomberman walks across the screen. fmod makes him start again from the
    // left (just outside the screen) when he leaves on the right
    const float walkX = std::fmod(elapsedMs / 25.f, VIEW_WIDTH + 64.f) - 32.f;
    bomberman->setPosition(glm::vec2(std::round(walkX), 76.f));
    bomberman->render();

    for (int i = 0; i < OPTION_COUNT; ++i)
    {
        const float y = 130.f + i * 18.f;
        const bool isSelected = (i == selected);
        text->renderCentered(OPTIONS[i], VIEW_WIDTH / 2.f, y, isSelected ? HIGHLIGHT_COLOR : MAIN_TEXT_COLOR);

        // A bomb points at the selected option, just left of its text
        if (isSelected)
            renderIcon(0, 2, glm::vec2(VIEW_WIDTH / 2.f - text->width(OPTIONS[i]) / 2.f - 22.f, y - 4.f));
    }

    text->renderCentered("UP/DOWN + ENTER or Z", VIEW_WIDTH / 2.f, 220.f, MAIN_TEXT_COLOR);
}

void Menu::renderInstructions()
{
    bigText->renderCentered("INSTRUCTIONS", VIEW_WIDTH / 2.f, 8.f, TITLE_COLOR);

    // Each call writes a line and moves down, so adding or removing a line
    // doesn't mean fixing the y of all the others by hand
    float y = 32.f;
    auto line = [&](const char *str, const glm::vec4 &color)
    {
        text->render(str, glm::vec2(12.f, y), color);
        y += 11.f;
    };

    line("DEFEAT ALL THE ENEMIES AND REACH THE DOOR", HIGHLIGHT_COLOR);
    line("BEFORE THE TIME RUNS OUT.", HIGHLIGHT_COLOR);
    y += 4.f;
    line("LEFT/RIGHT   MOVE", NORMAL_COLOR);
    line("UP OR Z      JUMP", NORMAL_COLOR);
    line("SPACE OR X   DROP A BOMB", NORMAL_COLOR);
    line("ESC          BACK TO THE MENU", NORMAL_COLOR);

    // Items with their icons, in two columns. The icon column is the same as
    // the ItemType value, like in the game
    y += 6.f;
    line("BREAK BUSHES TO FIND THESE POWER-UPS:", HEADER_COLOR);
    const char *names[] = { "BOMB UP: +1 BOMB", "FIRE UP: +1 FIRE RANGE", "HEART: SURVIVE A HIT", "SPEED UP: RUN FASTER" };
    for (int i = 0; i < 4; ++i)
    {
        const glm::vec2 pos(12.f + (i % 2) * 152.f, y + (i / 2) * 20.f);
        renderIcon(i, 4, pos);
        text->render(names[i], pos + glm::vec2(20.f, 4.f), NORMAL_COLOR);
    }
    y += 46.f;

    line("FENCES:", HEADER_COLOR);
    line("BLAST THE TOP CONTROL TO OPEN THE FENCE.", NORMAL_COLOR);
    line("FENCE BARS BLOCK EXPLOSIONS.", NORMAL_COLOR);
    y += 4.f;

    line("CHEATS:", HEADER_COLOR);
    line("G: GOD MODE   K: KILL ENEMIES   1-5: GO TO LEVEL", NORMAL_COLOR);
}

void Menu::renderCredits()
{
    bigText->renderCentered("CREDITS", VIEW_WIDTH / 2.f, 16.f, TITLE_COLOR);

    text->renderCentered("PROGRAMMING", VIEW_WIDTH / 2.f, 48.f, HEADER_COLOR);
    text->renderCentered("JUN ZHE XIA", VIEW_WIDTH / 2.f, 62.f, HIGHLIGHT_COLOR);
    text->renderCentered("DAIWEI CHEN", VIEW_WIDTH / 2.f, 74.f, HIGHLIGHT_COLOR);

    text->renderCentered("BASED ON", VIEW_WIDTH / 2.f, 98.f, HEADER_COLOR);
    text->renderCentered("POCKET BOMBERMAN (HUDSON SOFT, 1998)", VIEW_WIDTH / 2.f, 112.f, HIGHLIGHT_COLOR);

    text->renderCentered("SOUND", VIEW_WIDTH / 2.f, 136.f, HEADER_COLOR);
    text->renderCentered("MUSIC: POCKET BOMBERMAN OST (KHINSIDER)", VIEW_WIDTH / 2.f, 150.f, HIGHLIGHT_COLOR);
    text->renderCentered("SOUND EFFECTS: MADE WITH JSFXR", VIEW_WIDTH / 2.f, 162.f, HIGHLIGHT_COLOR);

    text->renderCentered("VIDEOJOCS - FIB - UPC", VIEW_WIDTH / 2.f, 186.f, NORMAL_COLOR);
    text->renderCentered("CURS 2026/27", VIEW_WIDTH / 2.f, 198.f, NORMAL_COLOR);

    text->renderCentered("PRESS ENTER/Z TO GO BACK", VIEW_WIDTH / 2.f, 220.f, NORMAL_COLOR);
}
