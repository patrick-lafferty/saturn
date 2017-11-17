#include <string.h>
__attribute__((section(".setup")))
void* prekernel_memset(void* destination, int fillByte, size_t count) {
    if (destination == nullptr) {
        return destination;
    }

    auto buffer = static_cast<unsigned char*>(destination);

    while (count--) {
        *buffer++ = fillByte;
    }

    return destination;
}

__attribute__((section(".text")))
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