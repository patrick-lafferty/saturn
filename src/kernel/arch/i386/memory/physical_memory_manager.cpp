#include "physical_memory_manager.h"
#include "scan_memory.h"
#include <stdio.h>
#include <string.h>

extern uint32_t __kernel_memory_start;
extern uint32_t __kernel_memory_end;

#if TARGET_PREKERNEL
#define MEMORY_NS Memory::
namespace MemoryPrekernel {
#else
#define MEMORY_NS
namespace Memory {
#endif

    PhysicalMemoryManager* currentPMM;

    PhysicalMemoryManager::PhysicalMemoryManager() {
        allocatedPages = 0;
        //currentPMM = this;
    }

    #if TARGET_PREKERNEL
    void PhysicalMemoryManager::initialize(const Kernel::MultibootInformation* info) {
        if (Kernel::hasValidMemoryMap(info)) {

            auto memoryMap = static_cast<MEMORY_NS MemoryMapRecord*>(reinterpret_cast<void*>(info->memoryMapAddress));
            auto count = info->memoryMapLength / sizeof(MEMORY_NS MemoryMapRecord);
            auto kernelStartAddress = reinterpret_cast<uint32_t>(&__kernel_memory_start);
            auto kernelEndAddress = reinterpret_cast<uint32_t>(&__kernel_memory_end);
            kernelEndAddress -= 0xD000'0000;
     
            for (auto i = 0u; i < count; i++) {
                auto& record = *memoryMap++;

                if (record.type == 1 && record.lowerLength >= PageSize) {
                    auto address = record.lowerBaseAddress;

                    if (address >= kernelStartAddress && address <= kernelEndAddress) {
                        address = (kernelEndAddress & 0xFFFFF000) + PageSize;
                    }

                    auto pages = record.lowerLength / PageSize;
                    totalPages += pages;
                    freePage(address, pages);
                }
            }
        }
        else 
        {
            freePage(0x8000, 1);
            totalPages++;
        }

        //printf("[PMM] Created %d pages, total physical memory: %dKB\n", totalPages, totalPages * PageSize / 1024);
    }
    #endif
    
    uintptr_t PhysicalMemoryManager::allocatePage(uint32_t count) {
        if (freePages < count) {
            return 0;
        }

        auto allocated = nextFreeAddress;

        allocatedPages += count;
        freePages -= count;

        return allocated;
    }

    void PhysicalMemoryManager::finishAllocation(uintptr_t pageAddress, uint32_t count) {
        //NOTE: can't do loop because page->nextFreePage isn't mapped
        //for demand paging, pageAddress should come from page fault
        for (uint32_t i = 0; i < count; i++) {
            auto page = static_cast<Page*>(reinterpret_cast<void*>(pageAddress & ~0xfff));
            nextFreeAddress = page->nextFreePage;
            
            #if TARGET_PREKERNEL
            prekernel_memset(page, 0, PageSize);
            #else
            memset(page, 0, PageSize);
            #endif
        }
    }

    void PhysicalMemoryManager::report() {
        printf("[PMM] Allocated: %d pages, %d KB; Free: %d pages, %d KB\n", 
            allocatedPages, allocatedPages * PageSize / 1024,
            freePages, freePages * PageSize / 1024);
    }

    void PhysicalMemoryManager::freePage(uintptr_t pageAddress, uint32_t count) {

        if (freePages == totalPages) {
            printf("[PMM] Tried to free an extra page: %x\n", pageAddress);
            return;
        }

        for (uint32_t i = 0; i < count; i++) {
            auto page = static_cast<Page volatile*>(reinterpret_cast<void*>(pageAddress & ~0xfff));

            page->nextFreePage = nextFreeAddress;
            nextFreeAddress = pageAddress;
            pageAddress += PageSize;
        }

        allocatedPages -= count;
        freePages += count;
    }
}