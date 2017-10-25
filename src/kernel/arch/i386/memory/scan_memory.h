#pragma once

#include <stdint.h>

namespace Memory {
    struct MemoryMapRecord {
        uint32_t size;
        uint32_t lowerBaseAddress;
        uint32_t higherBaseAddress;
        uint32_t lowerLength;
        uint32_t higherLength;
        uint32_t type;
    }__attribute__((packed));

    void scan(uintptr_t memoryMapAddress, uint32_t memoryMapLength);
}