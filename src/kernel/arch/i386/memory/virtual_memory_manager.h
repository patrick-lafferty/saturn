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

        /*
        ensures that the appropriate page table exists, and creates one
        if necessary
        */
        uintptr_t allocatePages(uint32_t count, uintptr_t startingVirtualAddress = 0);

        /*
        must only be called before paging is enabled
        */
        void map_unpaged(uintptr_t virtualAddress, uintptr_t physicalAddress, uint32_t pageCount = 0);

        /*
        maps the virtual address to the physical address

        note: the page table for the address must already exist, so
        allocatePages should be called before map
        */
        void map(uintptr_t virtualAddress, uintptr_t physicalAddress);

        /*
        enables paging and sets this VMM's directory as the current one
        reassigns directory to use the virtual address instead of physical
        */
        void activate();

    private:

        uintptr_t allocatePageTable(uintptr_t virtualAddress, int index);

        PhysicalMemoryManager& physicalManager;
        PageDirectory* directory;
        uintptr_t nextAddress {0};
    };
}