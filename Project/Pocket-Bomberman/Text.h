#ifndef _TEXT_INCLUDE
#define _TEXT_INCLUDE

#include <memory>
#include <string>
#include <glm/glm.hpp>

#include "Sprite.h"

// Text draws strings with a small 5x7 pixel font that we built in the code
// (no font file needed). Lowercase letters are drawn as uppercase.
// Game has two of them: one with 8 pixel letters and one with 16 for titles.
class Text
{
public:
    // cellSize is the pixel size of each character cell (8 = original size, 16 = double)
    void init(ShaderProgram &program, int cellSize);
    void free();

    // pos is the top-left corner of the first letter
    void render(const std::string &str,
                const glm::vec2 &pos,
                const glm::vec4 &color = glm::vec4(1.f));

    // Same, but the text is centered on centerX
    void renderCentered(const std::string &str,
                        float centerX,
                        float y,
                        const glm::vec4 &color = glm::vec4(1.f));

    // How many pixels wide the string is when drawn
    float width(const std::string &str) const;
    int getCellSize() const { return cellSize; }

private:
    ShaderProgram *program = nullptr;
    Texture fontTexture;
    std::unique_ptr<Sprite> sprite;
    int cellSize = 8;
};

#endif // _TEXT_INCLUDE
