#ifndef _TEXT_INCLUDE
#define _TEXT_INCLUDE

#include <memory>
#include <string>
#include <glm/glm.hpp>

#include "Sprite.h"

// Text draws strings with a small 5x7 pixel font that is built in code
// (no image needed). Lowercase letters are drawn as uppercase.
class Text
{
public:
    // cellSize is the on-screen size of each character cell (8 = original size)
    void init(ShaderProgram &program, int cellSize);
    void free();

    void render(const std::string &str,
                const glm::vec2 &pos,
                const glm::vec4 &color = glm::vec4(1.f));

    void renderCentered(const std::string &str,
                        float centerX,
                        float y,
                        const glm::vec4 &color = glm::vec4(1.f));

    float width(const std::string &str) const;
    int getCellSize() const { return cellSize; }

private:
    ShaderProgram *program = nullptr;
    Texture fontTexture;
    std::unique_ptr<Sprite> sprite;
    int cellSize = 8;
};

#endif // _TEXT_INCLUDE
