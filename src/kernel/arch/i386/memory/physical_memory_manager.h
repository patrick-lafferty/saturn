#pragma once

#include <stdint.h>
#include <multiboot.h>

namespace Memory {

    struct Page {
        uintptr_t nextFreePage;
    };

    const int PageSize {0x1000};

    class PhysicalMemoryManager {
    public:

        PhysicalMemoryManager(const Kernel::MultibootInformation* info);

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
        void freePage(uintptr_t pageAddress, uint32_t count);
        void report();

    private:

        uintptr_t nextFreeAddress {0};
        uint32_t totalPages {0};
        uint32_t allocatedPages {0};
        uint32_t freePages {0};
    };

}