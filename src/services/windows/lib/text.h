#pragma once
#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_GLYPH_H

namespace Window::Text {

    class Renderer {
    public:

        Renderer(FT_Library library, FT_Face face, uint32_t* frameBuffer);

        void drawText(char* text);

    private:

        FT_Library library;
        FT_Face face;
        uint32_t* frameBuffer;

    };

    Renderer* createRenderer(uint32_t* frameBuffer);
}