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

    struct BoundingBox {
        FT_BBox box;
        uint32_t width;
        uint32_t height;
    };

    struct TextLayout {
        std::vector<Glyph> glyphs;
        BoundingBox bounds;
    };
    
    class Renderer {
    public:

        Renderer(FT_Library library, FT_Face face, uint32_t* frameBuffer);

        //TODO: should be const TextLayout& but vector is missing cbegin and const iterators
        void drawText(TextLayout& layout, uint32_t x, uint32_t y);
        TextLayout layoutText(char* text, uint32_t allowedWidth);

    private:

        FT_Library library;
        FT_Face face;
        uint32_t* frameBuffer;
        uint32_t windowWidth;
        uint32_t windowHeight;
    };

    Renderer* createRenderer(uint32_t* frameBuffer);
}