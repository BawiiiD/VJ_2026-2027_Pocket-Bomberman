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
    // Vertex data for a quad (the size in screen coordinates is given by quadSize, 
    // and the size in the spritesheet is given by sizeInSpritesheet)
    // Each vertex has 4 floats: 2 for position and 2 for texture coordinates
    // Later we add the displacement of the texture coordinates for each keyframe in the animation
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

// Update the animation state of the sprite based on the elapsed time (deltaTime)
void Sprite::update(int deltaTime)
{
    // If there is no current animation or the animation has ended, do nothing
    if (currentAnimation < 0 || animationEnded)
        return;
    // If the current animation has no keyframes, do nothing
    if (animations[currentAnimation].keyframeDispl.empty())
        return;

    const float millisecondsPerFrame =
        animations[currentAnimation].millisecsPerKeyframe;

    // Animations without a speed never advance
    if (millisecondsPerFrame <= 0.f)
        return;

    timeAnimation += deltaTime;

    // While the accumulated time exceeds the duration of a single frame, advance to the next keyframe
    while (timeAnimation >= millisecondsPerFrame)
    {
        timeAnimation -= millisecondsPerFrame;
        ++currentKeyframe;

        const int numberOfFrames =
            static_cast<int>(
                animations[currentAnimation]
                    .keyframeDispl.size()
            );
        
        // If the current keyframe exceeds the number of frames in the animation, 
        // handle looping or ending the animation
        if (currentKeyframe >= numberOfFrames)
        {
            // If the animation is set to loop, reset to the first keyframe
            if (animationLoop[currentAnimation])
            {
                currentKeyframe = 0;
            }
            // If the animation is not set to loop, stay on the last keyframe and mark the animation as ended
            else
            {
                currentKeyframe = numberOfFrames - 1;
                animationEnded = true;
                break;
            }
        }

        // Update the texture coordinate displacement based on the current keyframe
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
        // Translate the modelview matrix to position the sprite correctly after flipping (because
        // flipping changes the origin of the sprite)
        // Scale the modelview matrix by -1 in the x direction to flip the sprite horizontally
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
        // Translate the modelview matrix to position the sprite at its current position
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

// Set the number of animations for the sprite and initialize the animation data structures
void Sprite::setNumberAnimations(int nAnimations)
{
    animations.clear();
    animations.resize(nAnimations);

    animationLoop.clear();
    animationLoop.resize(nAnimations, true);
}

// Set the speed of a specific animation in keyframes per second
void Sprite::setAnimationSpeed(int animId, int keyframesPerSec)
{
    if (animId < 0 || animId >= int(animations.size()))
        return;

    animations[animId].millisecsPerKeyframe =
        1000.f / float(keyframesPerSec);
}

// Add a keyframe to a specific animation, specifying the displacement of the sprite at that keyframe
void Sprite::addKeyframe(int animId,
                         const glm::vec2 &displacement)
{
    if (animId < 0 || animId >= int(animations.size()))
        return;

    animations[animId].keyframeDispl.push_back(displacement);
}

// Set whether a specific animation should loop or not
void Sprite::setAnimationLoop(int animId, bool loop)
{
    if (animId < 0 || animId >= int(animationLoop.size()))
        return;

    animationLoop[animId] = loop;
}

// Check if the current animation has finished playing (only relevant for non-looping animations)
bool Sprite::animationFinished() const
{
    return animationEnded;
}

// Change the current animation to a new one, resetting the keyframe and time counters
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

// Get the ID of the current animation
int Sprite::animation() const
{
    return currentAnimation;
}

// Set the position of the sprite in screen coordinates
void Sprite::setPosition(const glm::vec2 &pos)
{
    position = pos;
}

// Show a static frame of the spritesheet
void Sprite::setFrame(const glm::vec2 &texCoord)
{
    texCoordDispl = texCoord;
}

// Get the current position of the sprite in screen coordinates
glm::vec2 Sprite::getPosition() const
{
    return position;
}

// Set whether the sprite should be flipped horizontally or not
void Sprite::setFlipX(bool flip)
{
    flipX = flip;
}