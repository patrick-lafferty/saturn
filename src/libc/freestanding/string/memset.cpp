#include <string.h>

void* memset(void* destination, int fillByte, size_t count) {
    if (destination == nullptr) {
        return destination;
    }

    auto buffer = static_cast<unsigned char*>(destination);

    while (count--) {
        *buffer++ = fillByte;
    }

    return destination;
}