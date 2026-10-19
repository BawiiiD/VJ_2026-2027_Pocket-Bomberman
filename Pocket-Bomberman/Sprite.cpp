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

    glBufferData(
        GL_ARRAY_BUFFER,
        24 * sizeof(float),
        vertices,
        GL_STATIC_DRAW
    );

    posLocation = program->bindVertexAttribute(
        "position",
        2,
        4 * sizeof(float),
        0
    );

    texCoordLocation = program->bindVertexAttribute(
        "texCoord",
        2,
        4 * sizeof(float),
        (void *)(2 * sizeof(float))
    );

    texture = spritesheet;
    shaderProgram = program;

    this->quadSize = quadSize;

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
    if (currentAnimation < 0 || animationEnded)
        return;

    if (animations[currentAnimation].keyframeDispl.empty())
        return;

    timeAnimation += deltaTime;

    const float millisecondsPerFrame =
        animations[currentAnimation].millisecsPerKeyframe;

    while (timeAnimation >= millisecondsPerFrame)
    {
        timeAnimation -= millisecondsPerFrame;
        ++currentKeyframe;

        const int numberOfFrames =
            static_cast<int>(
                animations[currentAnimation]
                    .keyframeDispl.size()
            );

        if (currentKeyframe >= numberOfFrames)
        {
            if (animationLoop[currentAnimation])
            {
                currentKeyframe = 0;
            }
            else
            {
                currentKeyframe = numberOfFrames - 1;
                animationEnded = true;
                break;
            }
        }

        texCoordDispl =
            animations[currentAnimation]
                .keyframeDispl[currentKeyframe];
    }
}

void Sprite::render() const
{
    glm::mat4 modelview(1.f);

    if (flipX)
    {
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

    glDrawArrays(GL_TRIANGLES, 0, 6);

    glDisable(GL_TEXTURE_2D);
}

void Sprite::free()
{
    glDeleteVertexArrays(1, &vao);
    glDeleteBuffers(1, &vbo);
}

void Sprite::setNumberAnimations(int nAnimations)
{
    animations.clear();
    animations.resize(nAnimations);

    animationLoop.clear();
    animationLoop.resize(nAnimations, true);
}

void Sprite::setAnimationSpeed(int animId, int keyframesPerSec)
{
    if (animId < 0 || animId >= int(animations.size()))
        return;

    animations[animId].millisecsPerKeyframe =
        1000.f / float(keyframesPerSec);
}

void Sprite::addKeyframe(int animId,
                         const glm::vec2 &displacement)
{
    if (animId < 0 || animId >= int(animations.size()))
        return;

    animations[animId].keyframeDispl.push_back(displacement);
}

void Sprite::setAnimationLoop(int animId, bool loop)
{
    if (animId < 0 || animId >= int(animationLoop.size()))
        return;

    animationLoop[animId] = loop;
}

bool Sprite::animationFinished() const
{
    return animationEnded;
}

void Sprite::changeAnimation(int animId)
{
    if (animId < 0 || animId >= int(animations.size()))
        return;

    if (animations[animId].keyframeDispl.empty())
        return;

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

void Sprite::setPosition(const glm::vec2 &pos)
{
    position = pos;
}

void Sprite::setFlipX(bool flip)
{
    flipX = flip;
}