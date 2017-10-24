#include <string.h>

void* memcpy(void* destination, const void* source, size_t count) {
    auto lhs = static_cast<unsigned char*>(destination);
    auto rhs = static_cast<const unsigned char*>(source);

    for(size_t i = 0; i < count; i++) {
        lhs[i] = rhs[i];
    }

    return destination;
}