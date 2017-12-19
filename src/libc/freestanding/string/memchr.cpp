#include <string.h>

void* memchr(const void* s, int c, size_t n) {
    auto ptr = static_cast<const unsigned char*>(s);
    auto ch = (unsigned char)c;

    for (auto i = 0u; i < n; i++) {
        if (ptr[i] == ch) {
            auto p = const_cast<unsigned char*>(ptr + i);
            return static_cast<void*>(p);
        }
    }

    return nullptr;
}