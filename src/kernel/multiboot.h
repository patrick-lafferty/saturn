#pragma once

#include <stdint.h>

namespace Kernel {
    struct MultibootInformation {
        uint32_t flags;
        uint32_t amountLowerMemory;
        uint32_t amountHigherMemory;
        uint32_t bootDevice;
        uint32_t commandLine;
        uint32_t modulesCount;
        uint32_t modulesAddress;
        uint32_t unused_syms[4];
        uint32_t memoryMapLength;
        uint32_t memoryMapAddress;
    } __attribute__((packed));

__attribute__((section(".setup")))
    inline bool hasValidMemoryMap(const MultibootInformation* info) {
        return info->flags & (1 << 6);
    }
}