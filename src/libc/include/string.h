#pragma once

#include <stddef.h>

void* memset(void* destination, int fillByte, size_t count);
int memcmp(const void* first, const void* second, size_t count);
void* memmove(void* destination, const void* source, size_t count);