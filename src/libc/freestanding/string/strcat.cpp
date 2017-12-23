#include <string.h>

char* strcat(char* restrict dest, const char* restrict src) {
    while (*dest != '\0') {
        dest++;
    }

    while (*src != '\0') {
        *dest++ = *src++;
    }

    return dest;
}