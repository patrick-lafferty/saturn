#include "test.h"

#include <stdio.h>

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
        printf("[TWIN] %d\n", error);

        error = FT_New_Face(library,
            "/system/fonts/Inconsolata.ttf",
            0,
            &face);

        printf("[TWIN] %d\n", error);

        int ints[] = {-2, 99, 4, 0, -743, 0, 2, 69, 4};
        auto size = sizeof(ints) / sizeof(*ints);
        qsort(ints, size, sizeof(int), compare);

        for (auto i : ints) {
            printf("[TWIN] %d\n", i);
        }

    }
}