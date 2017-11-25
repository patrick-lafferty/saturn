#pragma once

#include <stddef.h>
#include <stdint.h>

namespace std {
    typedef size_t size_t;
    enum class align_val_t : size_t {};
}

//replaceable allocation functions
void* operator new(std::size_t count);
void* operator new[](std::size_t count);
void* operator new(std::size_t count, std::align_val_t alignment);

//non-allocating placement allocation functions
void* operator new (std::size_t count, void* ptr);

//replaceable usual deallocation functions
void operator delete(void* ptr);
void operator delete(void* ptr, std::align_val_t alignment);