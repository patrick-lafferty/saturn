#include <string.h>

size_t strlen(const char* str) {
    size_t length {0};

    while (*str != '\0') {
        length++;
        str++;
    }

    return length;
}

size_t strlen_s(const char* str, size_t strsz) {
    size_t length {0};

    if (str == nullptr) {
        return length;
    }

    auto remainingIterations = strsz;

    while (remainingIterations > 0 && *str != '\0') {
        length++;
        str++;
        remainingIterations--;
    }

    if (*str == '\0') {
        return length;
    }
    else {
        return strsz;
    }
}