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
#include <services/terminal/terminal.h>

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
            18 * 64,
            96,
            96
        );

        if (error != 0) {
            return nullptr; 
        }

        return new Renderer(library, face, frameBuffer);
    }

    Renderer::Renderer(FT_Library library, FT_Face face, uint32_t* frameBuffer)
        : library {library}, frameBuffer {frameBuffer} {
        windowWidth = 800;
        windowHeight = 600;
        faces[0] = face;

        char* fonts[] = {
            "/system/fonts/dejavu/DejaVuSans-Bold.ttf",
            "/system/fonts/dejavu/DejaVuSans-Oblique.ttf",
        };

        int i = 1;

        for (auto font : fonts) {
            FT_Face face;

            auto error = FT_New_Face(library,
                font,
                0,
                &face);

            if (error != 0) {
                continue;
            }

            error = FT_Set_Char_Size(
                face,
                0,
                18 * 64,
                96,
                96
            );

            if (error != 0) {
                continue;
            }

            faces[i] = face;
            i++;
        }
        
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
        origin.y = y;

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
            bitmap->top = origin.y + glyph.position.y;
            bitmap->left += glyph.position.x + origin.x;

            for (int row = 0; row < bitmap->bitmap.rows; row++) {
                auto y = row + bitmap->top;

                if (y == windowHeight)
                { 
                    continue;
                }

                for (int column = 0; column < bitmap->bitmap.pitch; column++) {
                    auto x = column + origin.x + glyph.position.x;

                    auto index = x + y * windowWidth;
                    auto val = *bitmap->bitmap.buffer++;
                    auto col = glyph.colour;
                    auto back = 0x00'20'20'20u;
                    auto f = blend(col, back, val);
                    frameBuffer[index] = f;
                }

            }
            
            /*TODO: this crashes, find out why
            FT_Done_Glyph(image);*/
        }

        if (layout.underline) {

            for (int row = 0; row < layout.underlineThickness; row++) {
                auto y = row + origin.y + layout.underlinePosition;

                for (int column = 0; column < layout.bounds.width; column++) {
                    auto x = column + origin.x;
                    auto index = x + y * windowWidth;
                    frameBuffer[index] = layout.glyphs[0].colour;
                }
            }
        }

    }

    uint32_t foreground = 0x00'64'95'EDu;
    uint32_t defaultForeground = 0x00'64'95'EDu;

    enum class ParseState {
        FindCode,
        FindCodeEx,
        FindChannel
    };

    uint32_t handleSelectGraphicRendition(char* buffer) {
        auto start = buffer;
        char* end {nullptr};
        long code {0};
        ParseState state {ParseState::FindCode};
        bool done = *buffer == '\0';
        uint32_t colour {0};
        auto channelIndex = 0;

        while (!done) {
            switch (state) {
                case ParseState::FindCode: {
                    code = strtol(buffer, &end, 10);

                    if (!(code == 38 || code == 48)) {
                        done = true;
                        break;
                    }

                    buffer = end;

                    if (*buffer != ';') {
                        done = true;
                        break;
                    }

                    buffer++;
                    state = ParseState::FindCodeEx;

                    break;
                }
                case ParseState::FindCodeEx: {
                    auto codeEx = strtol(buffer, &end, 10);

                    if (codeEx != 2) {
                        done = true;
                        break;
                    }

                    buffer = end;

                    if (*buffer != ';') {
                        done = true;
                        break;
                    }

                    buffer++;
                    state = ParseState::FindChannel;

                    break;
                }
                case ParseState::FindChannel: {
                    auto value = strtol(buffer, &end, 10);

                    buffer = end;

                    if (channelIndex < 1 && *buffer != ';') {
                        done = true;
                        break;
                    }

                    buffer++;
                    colour = (colour << 8) | value;
                    channelIndex++;

                    if (channelIndex == 3) {

                        if (code == 38) {
                            foreground = colour;
                        }

                        done = true;
                        break;                        
                    }

                    break;
                }
            }

        }

        return end - start + 1; //ends SGR ends with 'm', consume that
    } 

    uint32_t handleEscapeSequence(char* buffer) {
        auto start = buffer;

        switch (*buffer++) {
            case 'O': {
                //f1-f4...
                break;
            }
            case '[': {

                auto sequenceStart = buffer;
                bool validSequence {false};

                /*
                According to https://en.wikipedia.org/wiki/ANSI_escape_code,
                CSI is x*y*z, where:
                * is the kleene start
                x is a parameter byte from 0x30 to 0x3F
                y is an intermediate byte from 0x20 to 0x2F
                z is the final byte from 0x40 to 0x7E, and determines the sequence
                */

                while (*buffer != '\0') {
                    auto next = *buffer;

                    if (next >= 0x40 && next <= 0x7E) {
                        validSequence = true;
                        break;
                    }

                    buffer++;
                }

                if (!validSequence) {
                    return buffer - start;
                }

                switch(*buffer) {
                    case static_cast<char>(Terminal::CSIFinalByte::SelectGraphicRendition): {
                        return 1 + handleSelectGraphicRendition(sequenceStart);
                    }
                }

                break;
            }
        }

        return buffer - start;
    }

    TextLayout Renderer::layoutText(char* text, uint32_t allowedWidth, Style style, bool underline, uint32_t size) {
        TextLayout layout;
        layout.lines = 1;

        auto x = 0u;
        auto y = 0;

        FT_Face face = faces[static_cast<int>(style)];

        FT_Set_Char_Size(
            face,
            0,
            size * 64,
            96,
            96
        );

        bool kernTableAvailable = FT_HAS_KERNING(face);
        uint32_t previousIndex = 0;

        while (*text != '\0') { 

            if (*text == 27) {
                auto consumedChars = 0 + handleEscapeSequence(text + 1);
                text += consumedChars + 1;
                continue;
            }

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
            glyph.colour = foreground;

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

        if (underline) {
            layout.underline = true;
            layout.underlinePosition = (face->underline_position >> 6) + (face->size->metrics.height >> 6);
            layout.underlineThickness = face->underline_thickness >> 6;
        }

        return layout;
    }
}