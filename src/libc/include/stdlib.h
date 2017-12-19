#pragma once

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

void* malloc(size_t size);
void free(void* ptr);

void* aligned_alloc(size_t alignment, size_t size);
void* realloc(void* ptr, size_t size);

long strtol(const char* str, char** str_end, int base);

void qsort(void* base, size_t count, size_t size,
    int (*compare)(const void*, const void*));

#ifdef __cplusplus
}
#endif