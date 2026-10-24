#include <cctype>
#include <vector>

#include "Text.h"

namespace
{
    // The font texture has 16x8 cells of 8x8 pixels, one per ASCII code
    constexpr int CELL = 8;
    constexpr int COLUMNS = 16;
    constexpr int ROWS = 8;
    constexpr int GLYPH_WIDTH = 5;
    constexpr int GLYPH_HEIGHT = 7;

    // Horizontal distance between characters, in font pixels
    constexpr int ADVANCE = 6;

    struct Glyph
    {
        char character;
        const char *rows[GLYPH_HEIGHT];
    };

    const Glyph GLYPHS[] =
    {
        { 'A', { ".###.", "#...#", "#...#", "#####", "#...#", "#...#", "#...#" } },
        { 'B', { "####.", "#...#", "#...#", "####.", "#...#", "#...#", "####." } },
        { 'C', { ".###.", "#...#", "#....", "#....", "#....", "#...#", ".###." } },
        { 'D', { "####.", "#...#", "#...#", "#...#", "#...#", "#...#", "####." } },
        { 'E', { "#####", "#....", "#....", "####.", "#....", "#....", "#####" } },
        { 'F', { "#####", "#....", "#....", "####.", "#....", "#....", "#...." } },
        { 'G', { ".###.", "#...#", "#....", "#.###", "#...#", "#...#", ".####" } },
        { 'H', { "#...#", "#...#", "#...#", "#####", "#...#", "#...#", "#...#" } },
        { 'I', { ".###.", "..#..", "..#..", "..#..", "..#..", "..#..", ".###." } },
        { 'J', { "..###", "...#.", "...#.", "...#.", "...#.", "#..#.", ".##.." } },
        { 'K', { "#...#", "#..#.", "#.#..", "##...", "#.#..", "#..#.", "#...#" } },
        { 'L', { "#....", "#....", "#....", "#....", "#....", "#....", "#####" } },
        { 'M', { "#...#", "##.##", "#.#.#", "#.#.#", "#...#", "#...#", "#...#" } },
        { 'N', { "#...#", "#...#", "##..#", "#.#.#", "#..##", "#...#", "#...#" } },
        { 'O', { ".###.", "#...#", "#...#", "#...#", "#...#", "#...#", ".###." } },
        { 'P', { "####.", "#...#", "#...#", "####.", "#....", "#....", "#...." } },
        { 'Q', { ".###.", "#...#", "#...#", "#...#", "#.#.#", "#..#.", ".##.#" } },
        { 'R', { "####.", "#...#", "#...#", "####.", "#.#..", "#..#.", "#...#" } },
        { 'S', { ".####", "#....", "#....", ".###.", "....#", "....#", "####." } },
        { 'T', { "#####", "..#..", "..#..", "..#..", "..#..", "..#..", "..#.." } },
        { 'U', { "#...#", "#...#", "#...#", "#...#", "#...#", "#...#", ".###." } },
        { 'V', { "#...#", "#...#", "#...#", "#...#", "#...#", ".#.#.", "..#.." } },
        { 'W', { "#...#", "#...#", "#...#", "#.#.#", "#.#.#", "#.#.#", ".#.#." } },
        { 'X', { "#...#", "#...#", ".#.#.", "..#..", ".#.#.", "#...#", "#...#" } },
        { 'Y', { "#...#", "#...#", ".#.#.", "..#..", "..#..", "..#..", "..#.." } },
        { 'Z', { "#####", "....#", "...#.", "..#..", ".#...", "#....", "#####" } },
        { '0', { ".###.", "#...#", "#..##", "#.#.#", "##..#", "#...#", ".###." } },
        { '1', { "..#..", ".##..", "..#..", "..#..", "..#..", "..#..", ".###." } },
        { '2', { ".###.", "#...#", "....#", "...#.", "..#..", ".#...", "#####" } },
        { '3', { "####.", "....#", "....#", ".###.", "....#", "....#", "####." } },
        { '4', { "...#.", "..##.", ".#.#.", "#..#.", "#####", "...#.", "...#." } },
        { '5', { "#####", "#....", "####.", "....#", "....#", "#...#", ".###." } },
        { '6', { ".###.", "#....", "#....", "####.", "#...#", "#...#", ".###." } },
        { '7', { "#####", "....#", "...#.", "..#..", ".#...", ".#...", ".#..." } },
        { '8', { ".###.", "#...#", "#...#", ".###.", "#...#", "#...#", ".###." } },
        { '9', { ".###.", "#...#", "#...#", ".####", "....#", "....#", ".###." } },
        { ':', { ".....", "..#..", "..#..", ".....", "..#..", "..#..", "....." } },
        { '.', { ".....", ".....", ".....", ".....", ".....", ".##..", ".##.." } },
        { ',', { ".....", ".....", ".....", ".....", ".##..", "..#..", ".#..." } },
        { '!', { "..#..", "..#..", "..#..", "..#..", "..#..", ".....", "..#.." } },
        { '?', { ".###.", "#...#", "....#", "...#.", "..#..", ".....", "..#.." } },
        { '-', { ".....", ".....", ".....", "#####", ".....", ".....", "....." } },
        { '+', { ".....", "..#..", "..#..", "#####", "..#..", "..#..", "....." } },
        { '/', { "....#", "....#", "...#.", "..#..", ".#...", "#....", "#...." } },
        { '\'', { "..#..", "..#..", ".#...", ".....", ".....", ".....", "....." } },
        { '(', { "...#.", "..#..", ".#...", ".#...", ".#...", "..#..", "...#." } },
        { ')', { ".#...", "..#..", "...#.", "...#.", "...#.", "..#..", ".#..." } },
        { '>', { ".#...", "..#..", "...#.", "....#", "...#.", "..#..", ".#..." } },
        { '<', { "...#.", "..#..", ".#...", "#....", ".#...", "..#..", "...#." } },
        { '=', { ".....", ".....", "#####", ".....", "#####", ".....", "....." } },
    };

    // Builds an RGBA image with every glyph drawn in white over a transparent background
    std::vector<unsigned char> buildFontImage()
    {
        const int width = COLUMNS * CELL;
        const int height = ROWS * CELL;
        std::vector<unsigned char> image(width * height * 4, 0);

        for (const Glyph &glyph : GLYPHS)
        {
            // Cell coordinates of the glyph in the font texture
            const int cellX = (glyph.character % COLUMNS) * CELL;
            const int cellY = (glyph.character / COLUMNS) * CELL;

            // For each character of the glyph, if it's a '#', we set the corresponding pixel to white
            // Otherwise, we leave it transparent (0, 0, 0, 0)
            for (int y = 0; y < GLYPH_HEIGHT; ++y)
            {
                for (int x = 0; x < GLYPH_WIDTH; ++x)
                {
                    if (glyph.rows[y][x] != '#')
                        continue;

                    unsigned char *pixel =
                        &image[((cellY + y) * width + cellX + x) * 4];
                    pixel[0] = pixel[1] = pixel[2] = pixel[3] = 255;
                }
            }
        }

        return image;
    }
}

void Text::init(ShaderProgram &shaderProgram, int size)
{
    program = &shaderProgram;
    cellSize = size;

    std::vector<unsigned char> image = buildFontImage();
    fontTexture.loadFromRGBABuffer(&image[0], COLUMNS * CELL, ROWS * CELL);
    fontTexture.setWrapS(GL_CLAMP_TO_EDGE);
    fontTexture.setWrapT(GL_CLAMP_TO_EDGE);
    fontTexture.setMinFilter(GL_NEAREST);
    fontTexture.setMagFilter(GL_NEAREST);

    sprite.reset(Sprite::createSprite(
        glm::vec2(float(cellSize)),
        glm::vec2(1.f / COLUMNS, 1.f / ROWS),
        &fontTexture,
        program
    ));
}

void Text::free()
{
    sprite.reset();
}

void Text::render(const std::string &str,
                  const glm::vec2 &pos,
                  const glm::vec4 &color)
{
    if (!sprite)
        return;

    const float advance = float(cellSize * ADVANCE) / CELL;

    program->setUniform4f("color", color.r, color.g, color.b, color.a);

    glm::vec2 cursor = pos;
    for (char c : str)
    {
        // Convert lowercase to uppercase
        const int code = std::toupper(static_cast<unsigned char>(c));
        // Characters with no glyph are  drawn as spaces (so we still move the cursor)
        if (code != ' ' && code < COLUMNS * ROWS)
        {
            sprite->setFrame(glm::vec2(
                float(code % COLUMNS) / COLUMNS,
                float(code / COLUMNS) / ROWS
            ));
            sprite->setPosition(cursor);
            sprite->render();
        }
        cursor.x += advance;
    }

    // Reset the color to white
    program->setUniform4f("color", 1.f, 1.f, 1.f, 1.f);
}

void Text::renderCentered(const std::string &str,
                          float centerX,
                          float y,
                          const glm::vec4 &color)
{
    render(str, glm::vec2(centerX - width(str) / 2.f, y), color);
}

float Text::width(const std::string &str) const
{
    if (str.empty())
        return 0.f;

    // The last character does not need the spacing after it
    const float advance = float(cellSize * ADVANCE) / CELL;
    return advance * (str.size() - 1) + float(cellSize * GLYPH_WIDTH) / CELL;
}
