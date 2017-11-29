#pragma once

#include <stddef.h>

extern "C" { 
    void* prekernel_memset(void* destination, int fillByte, size_t count);
    void* memset(void* destination, int fillByte, size_t count);
    int memcmp(const void* first, const void* second, size_t count);
    void* memmove(void* destination, const void* source, size_t count);
    void* memcpy(void* destination, const void* source, size_t count);

    size_t strlen(const char* str);
    size_t strlen_s(const char* str, size_t strsz);

    int strcmp(const char* lhs, const char* rhs);
    int strncmp(const char* lhs, const char* rhs, size_t count);

    char* strrchr(const char* str, int ch);

    char* markWord(char* input, char separator = ' ');
}

