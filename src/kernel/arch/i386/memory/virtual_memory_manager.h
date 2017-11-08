#pragma once

#include <stdint.h>

namespace Memory {

    struct PageDirectory {
        uintptr_t pageTableAddresses[1024];
    };

    struct PageTable {
        uintptr_t pageAddresses[1024];
    };

    enum class PageTableFlags {
        Present = 1 << 0,
        AllowWrite = 1 << 1,
        AllowUserModeAccess = 1 << 2,
        WriteThrough = 1 << 3,
        CacheDisable = 1 << 4,
        Accessed = 1 << 5,
        Dirty = 1 << 6
    };

    //TODO: HACK: decide on how ISR can get access to current VMM
    extern class VirtualMemoryManager* currentVMM;

    enum class PageStatus {
        Allocated,
        Mapped,
        Invalid
    };

    class VirtualMemoryManager {
    public:

        VirtualMemoryManager(class PhysicalMemoryManager& physical);

        /*
        ensures that the appropriate page table exists, and creates one
        if necessary
        */
        uintptr_t allocatePages(uint32_t count, uint32_t flags = 0, uintptr_t startingVirtualAddress = 0);

        /*
        must only be called before paging is enabled
        */
        void map_unpaged(uintptr_t virtualAddress, uintptr_t physicalAddress, uint32_t pageCount, uint32_t flags);

        /*
        maps the virtual address to the physical address

        note: the page table for the address must already exist, so
        allocatePages should be called before map
        */
        void map(uintptr_t virtualAddress, uintptr_t physicalAddress, uint32_t flags = 0);

        /*
        enables paging and sets this VMM's directory as the current one
        reassigns directory to use the virtual address instead of physical
        */
        void activate();

        PageStatus getPageStatus(uintptr_t virtualAddress);

        void HACK_setNextAddress(uint32_t address);

    private:

        uintptr_t allocatePageTable(uintptr_t virtualAddress, int index);

        PhysicalMemoryManager& physicalManager;
        PageDirectory* directory;
        uintptr_t nextAddress {0};
        bool pagingActive {false};
    };
}