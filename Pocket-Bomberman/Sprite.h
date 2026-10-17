#ifndef _SPRITE_INCLUDE
#define _SPRITE_INCLUDE

#include <vector>
#include <glm/glm.hpp>
#include <GL/glew.h>

#include "Texture.h"
#include "ShaderProgram.h"
#include "AnimKeyframes.h"

using namespace std;

class Sprite
{
public:
    Sprite(const glm::vec2 &quadSize,
           const glm::vec2 &sizeInSpritesheet,
           Texture *spritesheet,
           ShaderProgram *program);

    ~Sprite();

    static Sprite *createSprite(const glm::vec2 &quadSize,
                                const glm::vec2 &sizeInSpritesheet,
                                Texture *spritesheet,
                                ShaderProgram *program);

    void update(int deltaTime);
    void render() const;
    void free();

    void setNumberAnimations(int nAnimations);
    void setAnimationSpeed(int animId, int keyframesPerSec);
    void addKeyframe(int animId, const glm::vec2 &frame);

    void setAnimationLoop(int animId, bool loop);
    bool animationFinished() const;

    void changeAnimation(int animId);
    int animation() const;

    void setFrame(const glm::vec2 &texCoord);

    void setPosition(const glm::vec2 &pos);
    glm::vec2 getPosition() const;
    void setFlipX(bool flip);

private:
    Texture *texture;
    ShaderProgram *shaderProgram;

    GLuint vao;
    GLuint vbo;
    GLint posLocation;
    GLint texCoordLocation;

    glm::vec2 position;
    glm::vec2 quadSize;

    int currentAnimation;
    int currentKeyframe;
    float timeAnimation;

    glm::vec2 texCoordDispl;

    // Animation data, each animation has a vector of keyframes and a speed
    vector<AnimKeyframes> animations;
    vector<bool> animationLoop;

    bool animationEnded;
    bool flipX;
};

#endif // _SPRITE_INCLUDE