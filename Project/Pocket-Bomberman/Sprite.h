#ifndef _SPRITE_INCLUDE
#define _SPRITE_INCLUDE

#include <vector>
#include <glm/glm.hpp>
#include <GL/glew.h>

#include "Texture.h"
#include "ShaderProgram.h"
#include "AnimKeyframes.h"

using namespace std;

// A Sprite is a rectangle (a "quad") that shows a piece of a texture.
// The quad is built once at (0,0); to move it we only change its position,
// and to change the picture we only move the window it looks at in the
// texture (texCoordDispl). That's how animations work: each frame is a
// different spot of the same image.
// It can also be flipped horizontally, so one set of frames works for
// facing left and right.
class Sprite
{
public:
    Sprite(const glm::vec2 &quadSize,
           const glm::vec2 &sizeInSpritesheet,
           Texture *spritesheet,
           ShaderProgram *program);

    ~Sprite();

    // quadSize is how big it's drawn (in game pixels) and sizeInSpritesheet
    // is how much of the texture one frame takes (in texture coordinates, 0..1)
    static Sprite *createSprite(const glm::vec2 &quadSize,
                                const glm::vec2 &sizeInSpritesheet,
                                Texture *spritesheet,
                                ShaderProgram *program);

    // Moves the current animation forward
    void update(int deltaTime);
    void render() const;

    void setNumberAnimations(int nAnimations);
    // Animation ids have to be in range, and every animation needs a speed and
    // at least one frame before it's played
    void setAnimationSpeed(int animId, int keyframesPerSec);
    void addKeyframe(int animId, const glm::vec2 &frame);

    // Animations loop by default. A non-looping one stops on its last frame
    // and then animationFinished() becomes true (we use it for deaths)
    void setAnimationLoop(int animId, bool loop);
    bool animationFinished() const;

    // Restarts from the first frame, so only call it when the animation really changes
    void changeAnimation(int animId);
    int animation() const;

    // Shows one fixed frame. Useful when the same sprite draws many different
    // things in one frame (letters of a text, HUD icons, explosion pieces)
    void setFrame(const glm::vec2 &texCoord);

    void setPosition(const glm::vec2 &pos);
    glm::vec2 getPosition() const;
    void setFlipX(bool flip);

private:
    Texture *texture;
    ShaderProgram *shaderProgram;

    // The quad's vertices live in the GPU (vbo), and the vao remembers how they're laid out
    GLuint vao;
    GLuint vbo;
    GLint posLocation;
    GLint texCoordLocation;

    glm::vec2 position;
    glm::vec2 quadSize;

    int currentAnimation;
    int currentKeyframe;
    // Time spent on the current frame
    float timeAnimation;

    // Where in the texture the current frame is
    glm::vec2 texCoordDispl;

    // Animation data, each animation has a vector of keyframes and a speed
    vector<AnimKeyframes> animations;
    vector<bool> animationLoop;

    bool animationEnded;
    bool flipX;
};

#endif // _SPRITE_INCLUDE
