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

    private:

        uintptr_t nextFreeAddress {0};
        uint32_t pageCount {0};
    };

}