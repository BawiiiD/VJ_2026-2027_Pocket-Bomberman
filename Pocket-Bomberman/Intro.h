#ifndef _INTRO_INCLUDE
#define _INTRO_INCLUDE

#include <memory>

#include "Sprite.h"
#include "Text.h"

// Story shown when the game starts (images/intro.png). It is a list of shots,
// each one with its picture, its animated sprites and two lines of text.
// Any key skips it.
class Intro
{
public:
    void init(ShaderProgram &program, Text &text);
    void shutdown();

    void update(int deltaTime);
    void render();
    void skip();
    bool isFinished() const;

private:
    enum class Shot
    {
        TOWER,      // Orange tower with lightning
        FLASH,      // Flash of light
        HERO,       // Bomberman in front of the light
        STONES,     // The five stones around the emblem, then they fly away
        SWORD,      // The sword with a star shining over its blade
        CLIFF,      // Bomberman looking at the tower from the cliff

        COUNT
    };

    void renderPicture();
    void renderLightning(int shotMs, float groundY);
    void renderStones(int shotMs, int shotDuration);
    void renderStar(int shotMs);
    void renderSprite(Sprite &sprite, const glm::vec2 &scenePos);
    // Pixel of images/intro.png to texture coordinates
    glm::vec2 texCoord(const glm::vec2 &pixel) const;

private:
    ShaderProgram *program = nullptr;
    Text *text = nullptr;
    Texture texture;

    std::unique_ptr<Sprite> picture;
    std::unique_ptr<Sprite> hero;
    std::unique_ptr<Sprite> heroBack;
    std::unique_ptr<Sprite> stone;
    std::unique_ptr<Sprite> lightning;
    std::unique_ptr<Sprite> star;

    int shot = 0;
    int shotMs = 0;
    bool finished = false;
};

#endif // _INTRO_INCLUDE
