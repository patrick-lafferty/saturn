#pragma once
#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_GLYPH_H
#include <vector>

namespace Window::Text {

    struct Glyph {
        FT_Vector position;
        FT_Glyph image;

        FT_Glyph copyImage();
    };
    
    class Renderer {
    public:

        Renderer(FT_Library library, FT_Face face, uint32_t* frameBuffer);

        void drawText(char* text);

    private:

        std::vector<Glyph> layoutText(char* text);

        FT_Library library;
        FT_Face face;
        uint32_t* frameBuffer;
        uint32_t windowHeight;

    };

    Renderer* createRenderer(uint32_t* frameBuffer);
}