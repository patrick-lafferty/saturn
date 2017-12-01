#include <string.h>


void* memmove(void* destination, const void* source, size_t count) {
    auto dest = static_cast<unsigned char*>(destination);
    auto src = static_cast<const unsigned char*>(source);

    if (dest < src) {
        for (auto i = 0u; i < count; i++) {
            dest[i] = src[i];
        }
    }
    else {
        for (auto i = count; i > 0; i--) {
            dest[i - 1] = src[i - 1];
        }        
    }

    return destination;
}