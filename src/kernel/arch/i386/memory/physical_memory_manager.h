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

        uintptr_t allocatePage(uint32_t count);
        void freePage(uintptr_t pageAddress, uint32_t count);
        void report();

    private:

        uintptr_t nextFreeAddress {0};
        uint32_t totalPages {0};
        uint32_t allocatedPages {0};
        uint32_t freePages {0};
    };

}