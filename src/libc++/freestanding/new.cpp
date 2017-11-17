#include <new.h>
#include <stdlib.h>

void* operator new(size_t count) {
    return malloc(count);
}