#pragma once

#include <stddef.h>

extern "C" void* malloc(size_t size);
extern "C" void free(void* ptr);

extern "C" void* aligned_alloc(size_t alignment, size_t size);

extern "C" long strtol(const char* restrict str, char** restrict str_end, int base);