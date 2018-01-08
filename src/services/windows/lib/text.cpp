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

    FT_Glyph Glyph::copyImage() {
        FT_Glyph glyph;

        auto error = FT_Glyph_Copy(image, &glyph);

        if (error) {
            return nullptr;
        }

        return glyph;
    }

    struct BoundingBox {
        FT_BBox box;
        uint32_t width;
        uint32_t height;
    };

    BoundingBox calculateBoundingBox(std::vector<Glyph>& glyphs) {
        BoundingBox box {{10000, 10000, -10000, -10000}};

        for (auto& glyph : glyphs) {
            FT_BBox glyphBounds;

            FT_Glyph_Get_CBox(glyph.image, ft_glyph_bbox_pixels, &glyphBounds);

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

        box.width = (box.box.xMax - box.box.xMin) ;/// 64;
        box.height = (box.box.yMax - box.box.yMin) ;/// 64;

        return box;
    }

    void Renderer::drawText(char* text) {
        auto layout = layoutText(text);
        auto bounds = calculateBoundingBox(layout);

        FT_Vector position;
        position.x = 0;
        position.y = (600 - (bounds.height / 1)) * 64;
        //position.y = ((600 - bounds.height) / 2) * 64;

        for(auto& glyph : layout) {

            auto image = glyph.copyImage();
            FT_Glyph_Transform(image, nullptr, &position);

            auto error = FT_Glyph_To_Bitmap(
                &image,
                FT_RENDER_MODE_NORMAL,
                nullptr,
                true);

            if (error) {
                continue;
            }

            auto bitmap = reinterpret_cast<FT_BitmapGlyph>(image);

            for (int row = 0; row < bitmap->bitmap.rows; row++) {
                auto y = (600 - bitmap->top) + row;//bitmap->top - (bitmap->bitmap.rows - row);

                for (int column = 0; column < bitmap->bitmap.pitch; column++) {
                    auto x = column + bitmap->left;

                    frameBuffer[x + y * 800] = *bitmap->bitmap.buffer++;
                }
            }

            position.x += image->advance.x >> 10;
            position.y += image->advance.y >> 10;

            //FT_Done_Glyph(image);
        }
    }

    std::vector<Glyph> Renderer::layoutText(char* text) {
        std::vector<Glyph> glyphs;

        auto x = 0u;
        auto y = 0u;

        while (*text != '\0') { 

            Glyph glyph;
            glyph.position.x = x;
            glyph.position.y = y;

            auto glyphIndex = FT_Get_Char_Index(face, *text); 
            auto error = FT_Load_Glyph(
                face,
                glyphIndex,
                0
            );

            FT_Get_Glyph(face->glyph, &glyph.image);
            FT_Glyph_Transform(glyph.image, nullptr, &glyph.position);

            x += face->glyph->advance.x >> 6;
            glyphs.push_back(glyph);
            text++;
        }

        return glyphs;
    }
}