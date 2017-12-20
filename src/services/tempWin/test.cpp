#include "test.h"

#include <stdio.h>
#include <system_calls.h>

#include <ft2build.h>
#include FT_FREETYPE_H

namespace win {

    int compare(const void* a, const void* b) {
        auto a1 = *(const int*)a;
        auto b1 = *(const int*)b;

        if (a1 < b1) return -1;
        if (a1 > b1) return 1;
        return 0;
    }

    void service() {
        FT_Library library;
        FT_Face face;

        auto error = FT_Init_FreeType(&library);

        if (error != 0) {
            printf("[TWIN] %d\n", error);

        }

        sleep(600);

        error = FT_New_Face(library,
            "/system/fonts/Inconsolata.ttf",
            0,
            &face);
        if (error != 0) {
            printf("[TWIN] %d\n", error);
        }

        while (true) {}
    }
}