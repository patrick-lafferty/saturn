#include "physical_memory_manager.h"
#include "scan_memory.h"
#include <stdio.h>

extern uint32_t __kernel_memory_start;
extern uint32_t __kernel_memory_end;

namespace Memory {

    PhysicalMemoryManager::PhysicalMemoryManager(const Kernel::MultibootInformation* info) {

        if (Kernel::hasValidMemoryMap(info)) {

            auto memoryMap = static_cast<MemoryMapRecord*>(reinterpret_cast<void*>(info->memoryMapAddress));
            auto count = info->memoryMapLength / sizeof(MemoryMapRecord);
            auto kernelStartAddress = reinterpret_cast<uint32_t>(&__kernel_memory_start);
            auto kernelEndAddress = reinterpret_cast<uint32_t>(&__kernel_memory_end);
     
            for (auto i = 0u; i < count; i++) {
                auto& record = *memoryMap++;

                if (record.type == 1 && record.lowerLength >= PageSize) {
                    auto address = record.lowerBaseAddress;

                    if (address >= kernelStartAddress && address <= kernelEndAddress) {
                        address = (kernelEndAddress & 0xFFFFF000) + PageSize;
                    }

                    auto pages = record.lowerLength / PageSize;
                    freePage(address, pages);
                    totalPages += pages;
                }
            }
        }
        else 
        {
            freePage(0x8000, 1);
            totalPages++;
        }

        printf("[PMM] Created %d pages, total physical memory: %dKB\n", totalPages, totalPages * PageSize / 1024);
        allocatedPages = 0;
    }
    
    uintptr_t PhysicalMemoryManager::allocatePage(uint32_t count) {
        auto allocated = nextFreeAddress;

        for (uint32_t i = 0; i < count; i++) {
            auto page = static_cast<Page*>(reinterpret_cast<void*>(nextFreeAddress));
            nextFreeAddress = page->nextFreePage;
        }

        allocatedPages += count;
        freePages -= count;

        return allocated;
    }

    void PhysicalMemoryManager::report() {
        printf("[PMM] Allocated: %d pages, %d KB; Free: %d pages, %d KB\n", 
            allocatedPages, allocatedPages * PageSize / 1024,
            freePages, freePages * PageSize / 1024);
    }

    void PhysicalMemoryManager::freePage(uintptr_t pageAddress, uint32_t count) {

        for (uint32_t i = 0; i < count; i++) {
            auto page = static_cast<Page*>(reinterpret_cast<void*>(pageAddress));

            page->nextFreePage = nextFreeAddress;
            nextFreeAddress = pageAddress;
            pageAddress += PageSize;
        }

        allocatedPages -= count;
        freePages += count;
    }
}