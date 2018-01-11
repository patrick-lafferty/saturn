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
#include "text.h"
#include <algorithm>
#include "debug.h"

namespace Window::Text {
    Renderer* createRenderer(uint32_t* frameBuffer) {
        FT_Library library;

        auto error = FT_Init_FreeType(&library);

        if (error != 0) {
            return nullptr;
        }

        FT_Face face;

        error = FT_New_Face(library,
            //"/system/fonts/OxygenMono-Regular.ttf",
            "/system/fonts/Oxygen-Sans.ttf",
            0,
            &face);

        if (error != 0) {
            return nullptr;
        }

        error = FT_Set_Char_Size(
            face,
            0,
            48 * 64,
            96,
            96
        );

        if (error != 0) {
            return nullptr; 
        }

        return new Renderer(library, face, frameBuffer);
    }

    Renderer::Renderer(FT_Library library, FT_Face face, uint32_t* frameBuffer)
        : library {library}, face {face}, frameBuffer {frameBuffer} {
        windowWidth = 800;
        windowHeight = 600;
    }

    FT_Glyph Glyph::copyImage() {
        FT_Glyph glyph;

        auto error = FT_Glyph_Copy(image, &glyph);

        if (error) {
            return nullptr;
        }

        return glyph;
    }

    BoundingBox calculateBoundingBox(std::vector<Glyph>& glyphs) {
        BoundingBox box {{10000, 10000, -10000, -10000}};

        for (auto& glyph : glyphs) {
            FT_BBox glyphBounds;

            FT_Glyph_Get_CBox(glyph.image, ft_glyph_bbox_pixels, &glyphBounds);
            //need to transform the bounding box by the glyph's position
            glyphBounds.xMax += glyph.position.x;
            glyphBounds.xMin += glyph.position.x;
            glyphBounds.yMax += glyph.position.y;
            glyphBounds.yMin += glyph.position.y;

            if (glyphBounds.xMax > box.box.xMax) {
                box.box.xMax = glyphBounds.xMax;
            }

            if (glyphBounds.xMin < box.box.xMin) {
                box.box.xMin = glyphBounds.xMin;
            }

            if (glyphBounds.yMax > box.box.yMax) {
                box.box.yMax = glyphBounds.yMax;
            }

            if (glyphBounds.yMin < box.box.yMin) {
                box.box.yMin = glyphBounds.yMin;
            }
        }

        if (box.box.xMin > box.box.xMax 
            || box.box.yMin > box.box.yMax) {
            return {};
        }

        box.width = (box.box.xMax - box.box.xMin); 
        box.height = (box.box.yMax - box.box.yMin);

        return box;
    }

    uint32_t blend(uint32_t source, uint32_t destination, uint8_t alpha) {

        auto sourceRed = (source >> 16) & 0xFF;
        auto sourceGreen = (source >> 8) & 0xFF;
        auto sourceBlue = source & 0xFF;

        auto destinationRed = (destination >> 16) & 0xFF;
        auto destinationGreen = (destination >> 8) & 0xFF;
        auto destinationBlue = destination & 0xFF;

        auto destinationAlpha = 255 - alpha;

        uint32_t red = ((sourceRed * alpha) + (destinationRed * destinationAlpha)) /  255;
        uint32_t green = ((sourceGreen * alpha) + (destinationGreen * destinationAlpha)) / 255;
        uint32_t blue = ((sourceBlue * alpha) + (destinationBlue * destinationAlpha)) / 255;

        auto result = (255 << 24) | (red << 16) | (green << 8) | blue;

        return result;
    }

    void Renderer::drawText(TextLayout& layout, uint32_t x, uint32_t y) {

        static uint32_t offset = 0;
        FT_Vector origin;
        origin.x = x;
        origin.y = y ;//+ layout.bounds.height;

        for(auto& glyph : layout.glyphs) {

            auto image = glyph.copyImage();
            
            auto error = FT_Glyph_To_Bitmap(
                &image,
                FT_RENDER_MODE_NORMAL,
                nullptr,
                true);

            if (error) {
                continue;
            }

            auto bitmap = reinterpret_cast<FT_BitmapGlyph>(image);
            auto delta = bitmap->top;
            bitmap->top = origin.y + glyph.position.y ;///*+ layout.maxAscent*/+ ((face->size->metrics.height - face->size->metrics.ascender) /64  - bitmap->top) /*- glyph.height*/;
            bitmap->left += glyph.position.x + origin.x;

            for (int row = 0; row < bitmap->bitmap.rows; row++) {
                auto y = row + bitmap->top;// - (layout.lines - 1) * layout.lineSpace;

                if (y == windowHeight)
                { 
                    continue;
                }

                for (int column = 0; column < bitmap->bitmap.pitch; column++) {
                    auto x = column + origin.x + glyph.position.x;

                    auto index = x + y * windowWidth;
                    auto val = *bitmap->bitmap.buffer++;
                    auto col = 0x00'64'95'EDu;
                    auto back = 0x00'20'20'20u;
                    auto f = blend(col, back, val);
                    frameBuffer[index] = f;
                }

            }

            /*TODO: this crashes, find out why
            FT_Done_Glyph(image);*/
        }

    }

    TextLayout Renderer::layoutText(char* text, uint32_t allowedWidth) {
        TextLayout layout;
        layout.lines = 1;

        auto x = 0u;
        auto y = 0;

        bool kernTableAvailable = FT_HAS_KERNING(face);
        uint32_t previousIndex = 0;

        while (*text != '\0') { 

            auto glyphIndex = FT_Get_Char_Index(face, *text); 
            auto error = FT_Load_Glyph(
                face,
                glyphIndex,
                FT_LOAD_DEFAULT
            );

            if (kernTableAvailable && previousIndex > 0) {
                FT_Vector kerning;

                FT_Get_Kerning(face, 
                    previousIndex, 
                    glyphIndex,
                    FT_KERNING_DEFAULT, 
                    &kerning);

                x += kerning.x >> 6;
            }

            auto widthRequired = (face->glyph->metrics.width >> 6) + (face->glyph->advance.x >> 6); 

            if ((x + widthRequired) >= allowedWidth) {
                x = 0;
                y += face->size->metrics.height >> 6;
                layout.lines++;
            }

            Glyph glyph;
            glyph.position.x = x;
            glyph.position.y = y;
            glyph.height = face->glyph->metrics.height >> 6;
            auto diff = (face->size->metrics.ascender - face->glyph->metrics.horiBearingY) / 64;
            glyph.position.y += diff;

            error = FT_Get_Glyph(face->glyph, &glyph.image);
            error = FT_Glyph_Transform(glyph.image, nullptr, &glyph.position);

            x += face->glyph->advance.x >> 6;

            layout.glyphs.push_back(glyph);
            text++;
            previousIndex = glyphIndex;
        }

        layout.bounds = calculateBoundingBox(layout.glyphs);
        layout.bounds.height += (face->size->metrics.height >> 6) / 2;
        layout.lineSpace = face->size->metrics.height >> 6;

        return layout;
    }
}