#pragma once

#include <stdint.h>
#include <multiboot.h>

/*
Two instances of this file are compiled to two separate objs, one
to be linked at a low address to be used for the prekernel before paging,
and one to be used by the real kernel after paging is setup. So there isn't
any duplicate symbols, have different namespaces for each obj.
*/

#if TARGET_PREKERNEL
namespace MemoryPrekernel {
#else
namespace Memory {
#endif

    struct Page {
        uintptr_t nextFreePage;
    };

#if TARGET_PREKERNEL
    struct ACPITableLocation {
        uint32_t startAddress;
        uint32_t pages;
    };

#endif

    const int PageSize {0x1000};

    //TODO: HACK: decide how ISR can get access to current PMM
    extern class PhysicalMemoryManager* currentPMM;
    class PhysicalMemoryManager {
    public:

        PhysicalMemoryManager();

        /*
        Reads the multiboot information to find out all the available
        memory space we can use
        */
        #if TARGET_PREKERNEL
        uint64_t initialize(const Kernel::MultibootInformation* info);
        #endif

        /*
        returns a physical address to a free page, and tracks stats.
        we can't get the address of the next free page yet,
        because its written in the page itself which needs to be first
        page mapped before it can be read. so, each call to 
        allocatePage(count) needs to be followed by finishAllocation(count)
        */
        uintptr_t allocatePage(uint32_t count);

        /*
        here pageAddress is the virtual address mapped from the physical
        address of the previous call to allocatePage. now we can read the
        page and get the physical address of the next page
        */
        void finishAllocation(uintptr_t pageAddress, uint32_t count);
        
        #if TARGET_PREKERNEL
        void freePage(uintptr_t pageAddress, uint32_t count);
        #else
        void freePage(uintptr_t virtualAddress, uintptr_t physicalAddress);
        #endif

        void report();
        uint32_t getFreePages();
        uint32_t getTotalPages();

    private:

        uintptr_t nextFreeAddress {0};
        uint32_t totalPages {0};
        uint32_t allocatedPages {0};
        uint32_t freePages {0};
    };

}