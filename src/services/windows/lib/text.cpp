#include "text.h"

namespace Window::Text {
    Renderer* createRenderer(uint32_t* frameBuffer) {
        FT_Library library;

        auto error = FT_Init_FreeType(&library);

        if (error != 0) {
            return nullptr;
        }

        FT_Face face;

        error = FT_New_Face(library,
            "/system/fonts/OxygenMono-Regular.ttf",
            0,
            &face);

        if (error != 0) {
            return nullptr;
        }

        error = FT_Set_Char_Size(
            face,
            0,
            16 * 64,
            300,
            300
        );

        if (error != 0) {
            return nullptr; 
        }

        return new Renderer(library, face, frameBuffer);
    }

    Renderer::Renderer(FT_Library library, FT_Face face, uint32_t* frameBuffer)
        : library {library}, face {face}, frameBuffer {frameBuffer} {

    }

    void Renderer::drawText(char* text) {

        while (*text != '\0') {
            auto glyphIndex = FT_Get_Char_Index(face, *text); 
            auto error = FT_Load_Glyph(
                face,
                glyphIndex,
                0
            );

            FT_Glyph glyph;
            FT_Get_Glyph(face->glyph, &glyph);

            error = FT_Render_Glyph(face->glyph, FT_RENDER_MODE_NORMAL);

            FT_Vector origin {};

            FT_Glyph_To_Bitmap(
                &glyph,
                FT_RENDER_MODE_NORMAL,
                &origin,
                1);

            auto slot = face->glyph;
            FT_BitmapGlyph  bit = (FT_BitmapGlyph)glyph;

            for (int y = 0; y < bit->bitmap.rows; y++) {
                for (int x = 0; x < bit->bitmap.pitch; x++) {
                    frameBuffer[x + y * 800] = *bit->bitmap.buffer++;
                }
            }

            text++;
        }
    }
}