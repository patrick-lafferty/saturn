#include <string.h>

char* strcpy(char* restrict dest, const char* restrict src) {
    while (*src != '\0') {
        *dest++ = *src++;
    }

    if (*src == '\0') {
        *dest = *src;
    }
    
    return dest;
}