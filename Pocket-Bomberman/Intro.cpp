#include <algorithm>
#include <cmath>
#include <iostream>

#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <SOIL.h>
#include <glm/gtc/matrix_transform.hpp>

#include "Intro.h"
#include "Game.h"

namespace
{
    // Everything is drawn twice as big as in images/intro.png
    constexpr float SCALE = 2.f;
    const glm::vec2 PICTURE_SIZE(160.f, 80.f);
    // Top of the picture on the screen; the text goes below it
    constexpr float PICTURE_TOP = 16.f;
    constexpr float TEXT_TOP = 192.f;

    constexpr int FADE_MS = 300;

    // In images/intro.png the sprites are below this row, over a green background
    constexpr int SPRITES_TOP = 520;
    const unsigned char KEY_COLOR[] = { 34, 177, 76 };

    // Position of each picture in images/intro.png (pixels)
    // The tower picture is one column narrower than the others (x = 28..186)
    const glm::vec2 TOWER_PICTURE(28.f, 18.f);
    const glm::vec2 CLIFF_PICTURE(27.f, 107.f);
    const glm::vec2 FLASH_PICTURE(28.f, 223.f);
    const glm::vec2 SWORD_PICTURE(28.f, 310.f);
    const glm::vec2 STONES_PICTURE(28.f, 424.f);

    // Sprites: size of a frame and position of each frame (pixels)
    const glm::vec2 HERO_SIZE(61.f, 64.f);
    const glm::vec2 HERO_FRAMES[] = { glm::vec2(53.f, 544.f), glm::vec2(120.f, 544.f) };
    const glm::vec2 HERO_BACK_SIZE(55.f, 48.f);
    const glm::vec2 HERO_BACK_FRAMES[] = { glm::vec2(57.f, 658.f), glm::vec2(125.f, 658.f) };
    const glm::vec2 STONE_SIZE(14.f, 16.f);
    const glm::vec2 STONE_FRAMES[] = { glm::vec2(55.f, 758.f), glm::vec2(72.f, 758.f), glm::vec2(90.f, 758.f) };
    const glm::vec2 LIGHTNING_SIZE(15.f, 32.f);
    const glm::vec2 LIGHTNING_FRAME(109.f, 742.f);
    const glm::vec2 STAR_SIZE(16.f, 16.f);
    const glm::vec2 STAR_FRAMES[] = { glm::vec2(124.f, 759.f), glm::vec2(142.f, 758.f) };

    // Inside the pictures (picture pixels)
    // Lightning to the left and to the right of the tower. It is drawn twice as
    // tall so that it goes from the sky down to the ground
    constexpr float LIGHTNING_LEFT_X = 12.f;
    constexpr float LIGHTNING_RIGHT_X = 62.f;
    constexpr float LIGHTNING_STRETCH = 2.f;
    constexpr float TOWER_GROUND = 58.f;
    constexpr float CLIFF_GROUND = 60.f;
    // Center of the emblem and distance of the stones to it
    const glm::vec2 EMBLEM_CENTER(81.f, 40.f);
    constexpr float STONE_RADIUS = 30.f;
    // At the end of the shot the stones fly away during this time
    constexpr int STONES_LEAVE_MS = 1200;
    // The star runs along the upper edge of the blade, from the hilt down to the tip
    const glm::vec2 BLADE_TOP(98.f, 26.f);
    const glm::vec2 BLADE_BOTTOM(30.f, 66.f);
    // Bomberman on the cliff and in front of the light
    const glm::vec2 HERO_POSITION(50.f, 8.f);
    const glm::vec2 HERO_BACK_POSITION(88.f, 16.f);

    struct ShotInfo
    {
        glm::vec2 picture;
        int durationMs;
        bool fadeIn;
        bool fadeOut;
        const char *line1;
        const char *line2;
    };

    // Same order as Intro::Shot
    const ShotInfo SHOTS[] =
    {
        { TOWER_PICTURE, 3500, true, false, "ONE STORMY NIGHT, A DARK TOWER", "APPEARED OUT OF NOWHERE..." },
        { FLASH_PICTURE, 600, false, false, "", "" },
        { FLASH_PICTURE, 3000, false, true, "BOMBERMAN SAW THE LIGHT", "AND SET OFF AT ONCE!" },
        { STONES_PICTURE, 4200, true, true, "THE FIVE POWER STONES THAT", "KEPT THE PEACE WERE SCATTERED!" },
        { SWORD_PICTURE, 3600, true, true, "ONLY THE LEGENDARY SWORD", "CAN STOP THE EVIL..." },
        { CLIFF_PICTURE, 4500, true, true, "BOMBERMAN'S ADVENTURE", "BEGINS NOW!" }
    };
    static_assert(sizeof(SHOTS) / sizeof(SHOTS[0]) == 6, "Every shot needs its info");
}

void Intro::init(ShaderProgram &shaderProgram, Text &smallText)
{
    program = &shaderProgram;
    text = &smallText;

    // The sprites have a green background: it becomes transparent. The pictures
    // keep their colors (the grass of the first one is that same green)
    int width, height, channels;
    unsigned char *image = SOIL_load_image("images/intro.png", &width, &height, &channels, SOIL_LOAD_RGBA);
    if (image == nullptr)
    {
        std::cout << "Could not load images/intro.png" << std::endl;
        finished = true;
        return;
    }
    for (int y = SPRITES_TOP; y < height; ++y)
        for (int x = 0; x < width; ++x)
        {
            unsigned char *pixel = &image[(y * width + x) * 4];
            if (pixel[0] == KEY_COLOR[0] && pixel[1] == KEY_COLOR[1] && pixel[2] == KEY_COLOR[2])
                pixel[3] = 0;
        }
    // Repeat the last column of the tower picture so that it is 160 pixels wide too
    for (int y = int(TOWER_PICTURE.y); y < int(TOWER_PICTURE.y + PICTURE_SIZE.y); ++y)
    {
        unsigned char *last = &image[(y * width + int(TOWER_PICTURE.x + PICTURE_SIZE.x) - 2) * 4];
        std::copy(last, last + 4, last + 4);
    }
    texture.loadFromRGBABuffer(image, width, height);
    SOIL_free_image_data(image);

    texture.setWrapS(GL_CLAMP_TO_EDGE);
    texture.setWrapT(GL_CLAMP_TO_EDGE);
    texture.setMinFilter(GL_NEAREST);
    texture.setMagFilter(GL_NEAREST);

    const glm::vec2 textureSize = glm::vec2(float(width), float(height));
    auto createSprite = [&](const glm::vec2 &size)
    {
        return std::unique_ptr<Sprite>(Sprite::createSprite(size * SCALE, size / textureSize, &texture, program));
    };
    picture = createSprite(PICTURE_SIZE);
    hero = createSprite(HERO_SIZE);
    heroBack = createSprite(HERO_BACK_SIZE);
    stone = createSprite(STONE_SIZE);
    lightning.reset(Sprite::createSprite(LIGHTNING_SIZE * SCALE * glm::vec2(1.f, LIGHTNING_STRETCH),
                                         LIGHTNING_SIZE / textureSize, &texture, program));
    star = createSprite(STAR_SIZE);
}

void Intro::shutdown()
{
    picture.reset();
    hero.reset();
    heroBack.reset();
    stone.reset();
    lightning.reset();
    star.reset();
}

void Intro::update(int deltaTime)
{
    if (finished)
        return;

    shotMs += deltaTime;
    if (shotMs >= SHOTS[shot].durationMs)
    {
        shotMs = 0;
        ++shot;
        if (shot >= int(Shot::COUNT))
            finished = true;
    }
}

void Intro::skip()
{
    finished = true;
}

bool Intro::isFinished() const
{
    return finished;
}

void Intro::render()
{
    if (finished)
        return;

    glm::mat4 projection = glm::ortho(0.f, float(VIEW_WIDTH), float(VIEW_HEIGHT), 0.f);
    glm::mat4 modelview(1.f);
    program->use();
    program->setUniformMatrix4f("projection", projection);
    program->setUniformMatrix4f("modelview", modelview);
    program->setUniform2f("texCoordDispl", 0.f, 0.f);

    // Fade from and to black
    const ShotInfo &info = SHOTS[shot];
    float brightness = 1.f;
    if (info.fadeIn)
        brightness = std::min(brightness, float(shotMs) / FADE_MS);
    if (info.fadeOut)
        brightness = std::min(brightness, float(info.durationMs - shotMs) / FADE_MS);
    program->setUniform4f("color", brightness, brightness, brightness, 1.f);

    // The sprites can't go out of the picture (the stones fly away)
    glEnable(GL_SCISSOR_TEST);
    glScissor(0, int((VIEW_HEIGHT - PICTURE_TOP - PICTURE_SIZE.y * SCALE) * PIXEL_SCALE),
              SCREEN_WIDTH, int(PICTURE_SIZE.y * SCALE * PIXEL_SCALE));

    renderPicture();

    switch (Shot(shot))
    {
    case Shot::TOWER:
    case Shot::CLIFF:
        renderLightning(shotMs, Shot(shot) == Shot::TOWER ? TOWER_GROUND : CLIFF_GROUND);
        if (Shot(shot) == Shot::CLIFF)
        {
            heroBack->setFrame(texCoord(HERO_BACK_FRAMES[(shotMs / 250) % 2]));
            renderSprite(*heroBack, HERO_BACK_POSITION);
        }
        break;
    case Shot::HERO:
        hero->setFrame(texCoord(HERO_FRAMES[(shotMs / 250) % 2]));
        renderSprite(*hero, HERO_POSITION);
        break;
    case Shot::STONES:
        renderStones(shotMs, info.durationMs);
        break;
    case Shot::SWORD:
        renderStar(shotMs);
        break;
    default:
        break;
    }

    glDisable(GL_SCISSOR_TEST);

    const glm::vec4 textColor(brightness, brightness, brightness, 1.f);
    text->renderCentered(info.line1, VIEW_WIDTH / 2.f, TEXT_TOP, textColor);
    text->renderCentered(info.line2, VIEW_WIDTH / 2.f, TEXT_TOP + 14.f, textColor);
    text->render("ENTER: SKIP", glm::vec2(VIEW_WIDTH - 4.f - text->width("ENTER: SKIP"), VIEW_HEIGHT - 12.f),
                 glm::vec4(0.4f, 0.4f, 0.4f, 1.f));
}

void Intro::renderPicture()
{
    picture->setFrame(texCoord(SHOTS[shot].picture));
    picture->setPosition(glm::vec2(0.f, PICTURE_TOP));
    picture->render();
}

void Intro::renderLightning(int ms, float groundY)
{
    // Short flashes, first on the left of the tower and then on the right
    const int t = ms % 1100;
    const bool left = (t >= 100 && t < 180) || (t >= 260 && t < 320);
    const bool right = (t >= 600 && t < 680) || (t >= 760 && t < 820);

    // The bottom of the lightning touches the ground; the top is cut by the picture
    const float top = groundY - LIGHTNING_SIZE.y * LIGHTNING_STRETCH;
    lightning->setFrame(texCoord(LIGHTNING_FRAME));
    if (left)
        renderSprite(*lightning, glm::vec2(LIGHTNING_LEFT_X, top));
    if (right)
        renderSprite(*lightning, glm::vec2(LIGHTNING_RIGHT_X, top));
}

void Intro::renderStones(int ms, int duration)
{
    // In the last part of the shot the stones fly away from the emblem, faster and faster
    float radius = STONE_RADIUS;
    const int leaveStart = duration - STONES_LEAVE_MS;
    if (ms > leaveStart)
    {
        const float progress = float(ms - leaveStart) / STONES_LEAVE_MS;
        radius += 200.f * progress * progress;
    }

    for (int i = 0; i < 5; ++i)
    {
        // Pentagon starting at the top, each stone spinning with a different phase
        const float angle = glm::radians(-90.f + 72.f * i);
        const glm::vec2 center = EMBLEM_CENTER + radius * glm::vec2(std::cos(angle), std::sin(angle));

        stone->setFrame(texCoord(STONE_FRAMES[(ms / 120 + i) % 3]));
        renderSprite(*stone, center - STONE_SIZE / 2.f);
    }
}

void Intro::renderStar(int ms)
{
    // The star runs along the blade and waits a bit before running again
    constexpr int RUN_MS = 900;
    constexpr int CYCLE_MS = 1400;
    const int t = ms % CYCLE_MS;
    if (t >= RUN_MS)
        return;

    const glm::vec2 center = glm::mix(BLADE_TOP, BLADE_BOTTOM, float(t) / RUN_MS);
    star->setFrame(texCoord(STAR_FRAMES[(ms / 100) % 2]));
    renderSprite(*star, center - STAR_SIZE / 2.f);
}

glm::vec2 Intro::texCoord(const glm::vec2 &pixel) const
{
    return pixel / glm::vec2(float(texture.width()), float(texture.height()));
}

void Intro::renderSprite(Sprite &sprite, const glm::vec2 &scenePos)
{
    // From picture pixels to screen coordinates
    sprite.setPosition(glm::round(scenePos * SCALE) + glm::vec2(0.f, PICTURE_TOP));
    sprite.render();
}
