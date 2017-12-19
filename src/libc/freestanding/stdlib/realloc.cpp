#include <stdlib.h>
#include <string.h>

void* realloc(void* ptr, size_t size) {
    /*
    TODO: see if we can expand/contract the area, see cppreference:realloc a)
    */
    auto replacement = malloc(size);
    memcpy(replacement, ptr, size);
    free(ptr);

    return replacement;
}