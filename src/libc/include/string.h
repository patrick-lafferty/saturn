#pragma once

#include <stddef.h>

extern "C" void* memset(void* destination, int fillByte, size_t count);
extern "C" int memcmp(const void* first, const void* second, size_t count);
extern "C" void* memmove(void* destination, const void* source, size_t count);
extern "C" void* memcpy(void* destination, const void* source, size_t count);