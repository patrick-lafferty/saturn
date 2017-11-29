#include <string.h>

char* strrchr(const char* str, int ch) {
    const char* lastInstance = nullptr;

    while (*str != '\0') {
        if (static_cast<unsigned char>(*str) == static_cast<unsigned char>(ch)) {
            lastInstance = str;
        }

        str++;
    }

    return const_cast<char*>(lastInstance);
}