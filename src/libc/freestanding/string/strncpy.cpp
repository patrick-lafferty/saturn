#include <string.h>

char* strncpy(char* restrict dest, const char* restrict src, size_t count) {
    while (*src != '\0' && count > 0) {
        *dest++ = *src++;
        count--;
    }

    if (*src == '\0') {
        *dest = *src;
    }

    if (count > 0) {
        *dest++ = '\0';
        count--;
    }
    
    return dest;
}