#pragma once

#include <stdint.h>

namespace Memory {

    struct PageDirectory {
        uintptr_t pageTableAddresses[1024];
    };

    struct PageTable {
        uintptr_t pageAddresses[1024];
    };

    class VirtualMemoryManager {
    public:

        VirtualMemoryManager(class PhysicalMemoryManager& physical);

        uintptr_t allocatePages(uint32_t count);
        void map(uintptr_t virtualAddress, uintptr_t physicalAddress, uint32_t pageCount = 0);
        void activate();

    private:

        uintptr_t allocatePageTable(uintptr_t virtualAddress, int index);

        PhysicalMemoryManager& physicalManager;
        PageDirectory* directory;
        uintptr_t nextAddress {0};
    };
}