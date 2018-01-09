#include "text.h"
#include <algorithm>

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
            24 * 64,
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

    void Renderer::drawText(TextLayout& layout, uint32_t x, uint32_t y) {

        static uint32_t offset = 0;
        FT_Vector origin;
        origin.x = x;
        origin.y = (windowHeight - layout.bounds.height) - std::min(y, windowHeight - layout.bounds.height);

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
            bitmap->top += glyph.position.y + origin.y;
            bitmap->left += glyph.position.x + origin.x;

            for (int row = 0; row < bitmap->bitmap.rows; row++) {
                auto y = (windowHeight - bitmap->top) + row;

                for (int column = 0; column < bitmap->bitmap.pitch; column++) {
                    auto x = column + bitmap->left;

                    auto index = x + y * windowWidth;
                    frameBuffer[index] = *bitmap->bitmap.buffer++;
                }
            }

            /*TODO: this crashes, find out why
            FT_Done_Glyph(image);*/
        }

    }

    TextLayout Renderer::layoutText(char* text, uint32_t allowedWidth) {
        TextLayout layout;

        auto x = 0u;
        auto y = 0;

        bool kernTableAvailable = FT_HAS_KERNING(face);
        uint32_t previousIndex = 0;

        while (*text != '\0') { 

            auto glyphIndex = FT_Get_Char_Index(face, *text); 
            auto error = FT_Load_Glyph(
                face,
                glyphIndex,
                0
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
                y -= face->size->metrics.height >> 6;
            }

            Glyph glyph;
            glyph.position.x = x;
            glyph.position.y = y;

            FT_Get_Glyph(face->glyph, &glyph.image);
            FT_Glyph_Transform(glyph.image, nullptr, &glyph.position);

            x += face->glyph->advance.x >> 6;

            layout.glyphs.push_back(glyph);
            text++;
            previousIndex = glyphIndex;
        }

        layout.bounds = calculateBoundingBox(layout.glyphs);

        return layout;
    }
}