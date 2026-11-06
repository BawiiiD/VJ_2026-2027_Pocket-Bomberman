#include <glm/gtc/matrix_transform.hpp>

#include "Sprite.h"

Sprite *Sprite::createSprite(const glm::vec2 &quadSize,
                             const glm::vec2 &sizeInSpritesheet,
                             Texture *spritesheet,
                             ShaderProgram *program)
{
    return new Sprite(
        quadSize,
        sizeInSpritesheet,
        spritesheet,
        program
    );
}

Sprite::Sprite(const glm::vec2 &quadSize,
               const glm::vec2 &sizeInSpritesheet,
               Texture *spritesheet,
               ShaderProgram *program)
{
    // The quad is two triangles. Each vertex has 4 floats: its position (x, y)
    // and its texture coordinate (u, v). The texture part only covers ONE frame,
    // starting at (0,0); the shader adds texCoordDispl to move it to the frame
    // we want, so the same vertices work for every frame of every animation
    float vertices[24] = {
        0.f,          0.f,           0.f,                  0.f,
        quadSize.x,   0.f,           sizeInSpritesheet.x,  0.f,
        quadSize.x,   quadSize.y,    sizeInSpritesheet.x,  sizeInSpritesheet.y,

        0.f,          0.f,           0.f,                  0.f,
        quadSize.x,   quadSize.y,    sizeInSpritesheet.x,  sizeInSpritesheet.y,
        0.f,          quadSize.y,    0.f,                  sizeInSpritesheet.y
    };

    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);

    glGenBuffers(1, &vbo);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);

    // The vertices never change (STATIC_DRAW), so they're uploaded only once
    glBufferData(
        GL_ARRAY_BUFFER,
        24 * sizeof(float),
        vertices,
        GL_STATIC_DRAW
    );

    // Every vertex is 4 floats long: the position is the first 2...
    posLocation = program->bindVertexAttribute(
        "position",
        2,
        4 * sizeof(float),
        0
    );

    // ...and the texture coordinate the other 2
    texCoordLocation = program->bindVertexAttribute(
        "texCoord",
        2,
        4 * sizeof(float),
        (void *)(2 * sizeof(float))
    );

    texture = spritesheet;
    shaderProgram = program;

    this->quadSize = quadSize;

    // -1 = no animation yet (sprites that only use setFrame stay like this)
    currentAnimation = -1;
    currentKeyframe = 0;
    timeAnimation = 0.f;

    texCoordDispl = glm::vec2(0.f);
    position = glm::vec2(0.f);

    animationEnded = false;
    flipX = false;
}

Sprite::~Sprite()
{
    glDeleteVertexArrays(1, &vao);
    glDeleteBuffers(1, &vbo);
}

void Sprite::update(int deltaTime)
{
    // Nothing to animate, or it's a non-looping animation that already ended
    if (currentAnimation < 0 || animationEnded)
        return;
    const AnimKeyframes &animation = animations[currentAnimation];
    const float millisecondsPerFrame = animation.millisecsPerKeyframe;
    const int numberOfFrames = int(animation.keyframeDispl.size());

    // An animation without a speed (setAnimationSpeed never called) has 0 ms per
    // frame, and the loop below would never end. One without frames has nothing
    // to show. In both cases we just don't animate
    if (millisecondsPerFrame <= 0.f || numberOfFrames == 0)
        return;

    timeAnimation += deltaTime;

    // A "while" and not an "if": after a slow frame we may have to skip more
    // than one keyframe to stay on time
    while (timeAnimation >= millisecondsPerFrame)
    {
        timeAnimation -= millisecondsPerFrame;
        ++currentKeyframe;

        if (currentKeyframe >= numberOfFrames)
        {
            if (animationLoop[currentAnimation])
            {
                currentKeyframe = 0;
            }
            // A non-looping animation stays on its last frame and tells
            // everyone it's done (the player uses it to know when he's dead)
            else
            {
                currentKeyframe = numberOfFrames - 1;
                animationEnded = true;
                break;
            }
        }

        texCoordDispl = animation.keyframeDispl[currentKeyframe];
    }
}

void Sprite::render() const
{
    // The modelview matrix moves the quad from (0,0) to where the sprite is
    glm::mat4 modelview(1.f);

    if (flipX)
    {
        // To mirror it we scale x by -1. That flips the quad around its left
        // edge, so it would end up to the left of its position; moving it one
        // width further to the right first puts it back in place
        modelview = glm::translate(
            modelview,
            glm::vec3(
                position.x + quadSize.x,
                position.y,
                0.f
            )
        );

        modelview = glm::scale(
            modelview,
            glm::vec3(-1.f, 1.f, 1.f)
        );
    }
    else
    {
        modelview = glm::translate(
            modelview,
            glm::vec3(position.x, position.y, 0.f)
        );
    }

    shaderProgram->setUniformMatrix4f(
        "modelview",
        modelview
    );

    // Tells the shader which frame of the texture to show
    shaderProgram->setUniform2f(
        "texCoordDispl",
        texCoordDispl.x,
        texCoordDispl.y
    );

    glEnable(GL_TEXTURE_2D);
    texture->use();

    glBindVertexArray(vao);

    glEnableVertexAttribArray(posLocation);
    glEnableVertexAttribArray(texCoordLocation);

    // 6 vertices = the 2 triangles of the quad
    glDrawArrays(GL_TRIANGLES, 0, 6);

    glDisable(GL_TEXTURE_2D);
}

void Sprite::setNumberAnimations(int nAnimations)
{
    animations.assign(nAnimations, AnimKeyframes());
    animationLoop.assign(nAnimations, true);
}

// We think in "frames per second" because it's easier, but we store how
// many milliseconds each frame lasts, which is what update() needs
void Sprite::setAnimationSpeed(int animId, int keyframesPerSec)
{
    animations[animId].millisecsPerKeyframe =
        1000.f / float(keyframesPerSec);
}

// displacement = texture coordinates of the top-left corner of the frame
void Sprite::addKeyframe(int animId,
                         const glm::vec2 &displacement)
{
    animations[animId].keyframeDispl.push_back(displacement);
}

void Sprite::setAnimationLoop(int animId, bool loop)
{
    animationLoop[animId] = loop;
}

bool Sprite::animationFinished() const
{
    return animationEnded;
}

// Starts the animation from its first frame. That's why the entities check
// that the animation really changed before calling this: calling it every
// frame would keep the animation stuck on frame 0
void Sprite::changeAnimation(int animId)
{
    currentAnimation = animId;
    currentKeyframe = 0;
    timeAnimation = 0.f;
    animationEnded = false;

    texCoordDispl =
        animations[animId].keyframeDispl[0];
}

int Sprite::animation() const
{
    return currentAnimation;
}

// Position of the top-left corner, in game pixels
void Sprite::setPosition(const glm::vec2 &pos)
{
    position = pos;
}

// Shows a fixed frame. It doesn't change the animation, so the next update()
// of an animated sprite will overwrite it
void Sprite::setFrame(const glm::vec2 &texCoord)
{
    texCoordDispl = texCoord;
}

glm::vec2 Sprite::getPosition() const
{
    return position;
}

void Sprite::setFlipX(bool flip)
{
    flipX = flip;
}
