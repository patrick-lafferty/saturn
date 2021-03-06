/*
Copyright (c) 2018, Patrick Lafferty
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

    * Redistributions of source code must retain the above copyright
      notice, this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright
      notice, this list of conditions and the following disclaimer in the
      documentation and/or other materials provided with the distribution.
    * Neither the name of the copyright holder nor the names of its 
      contributors may be used to endorse or promote products derived from 
      this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR 
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/
#pragma once
#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_GLYPH_H
#include <vector>
#include <optional>
#include <array>

namespace Apollo {
    class Window;

    namespace Elements {
        struct Bounds;
    }
}

namespace Apollo::Text {

    /*
    Represents a single printable character
    */
    struct Glyph {
        FT_Vector position;
        FT_Glyph image {nullptr};
        uint32_t height;
        uint32_t colour;
        uint8_t index;
        uint32_t glyphIndex;
        uint32_t advance;
        uint32_t width;

        FT_Glyph copyImage();

        bool isValid() {
            return image != nullptr;
        }
    };

    struct BoundingBox {
        FT_BBox box;
        uint32_t width {0};
        uint32_t height {0};
    };

    /*
    Stores the info necessary to render some text.
    If the text doesn't change then this can be cached
    and reused
    */
    struct TextLayout {
        std::vector<Glyph> glyphs;
        BoundingBox bounds;
        uint32_t lineSpace;
        uint32_t lines;
        bool underline {false};
        uint32_t underlinePosition;
        uint32_t underlineThickness;
    };

    enum class Style {
        Normal,
        Bold,
        Italic
    };

    struct Cache {
        FT_Face face;
        Style style;
        uint32_t size;
        std::array<Glyph, 128> glyphs;
        std::array<FT_BBox, 128> cachedBoundingBoxes;
    };

    class FaceCache {
    public:

        void addCache(FT_Face face, Style style, uint32_t size);
        std::optional<Cache*> getGlyphCache(Style style, uint32_t size);

    private:

        std::vector<Cache> cachedFaces;
    };

    /*
    Renderer handles the layout, positioning and rendering of text
    into a window's framebuffer
    */
    class Renderer {
    public:

        Renderer(FT_Library library, FT_Face face, Window* window);

        /*
        Renders a prepared text layout to the stored window's framebuffer,
        with alpha blending.
        */
        void drawText(const TextLayout& layout, 
            const Apollo::Elements::Bounds& bounds, 
            const Apollo::Elements::Bounds& clip, 
            uint32_t backgroundColour = 0);

        /*
        Prepares each glyph's bitmap with FreeType, positions each
        glyph with optional kerning, and applies line wrapping.

        Allows the use of ANSI escape sequences in text to change
        the colour of glyphs.
        */
        TextLayout layoutText(const char* text, 
            uint32_t allowedWidth, 
            uint32_t colour,
            Style style = Style::Normal, 
            bool underline = false, 
            uint32_t size = 18);

    private:

        BoundingBox calculateBoundingBox(std::vector<Glyph>& glyphs, std::array<FT_BBox, 128>& cachedBoundingBoxes);
        FT_Face loadFont(uint32_t index, uint32_t size);

        FT_Library library;
        uint32_t* frameBuffer;
        Window* window;
        uint32_t windowWidth;
        uint32_t windowHeight;

        const int MaxCachedGlyphIndex {128};

        FaceCache faceCache;
    };

    Renderer* createRenderer(Window* window);
}