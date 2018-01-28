/*
Copyright (c) 2017, Patrick Lafferty
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

    * Redistributions of source code must retain the above copyright
      notice, this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright
      notice, this list of conditions and the following disclaimer in the
      documentation and/or other materials provided with the distribution.
    * Neither the name of the copyright holder nor the names of its 
      contributors may be used to endorse or promote products derived from 
      this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR 
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/
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
    }

    #if TARGET_PREKERNEL
    uint64_t PhysicalMemoryManager::initialize(const Kernel::MultibootInformation* info) {

        uint64_t acpiLocation{};
        uint64_t possibleACPILocaton {};

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
                    auto pages = record.lowerLength / PageSize;

                    if (address >= kernelStartAddress && address <= kernelEndAddress) {
                        auto end = (kernelEndAddress & 0xFFFFF000) + PageSize;
                        pages -= (1 + (end - address) / PageSize);
                        address = end; 
                    }

                    allocatedPages += pages;
                    totalPages += pages;
                    freePage(address, pages);
                }
                else if (record.type == 3) {
                    acpiLocation = static_cast<uint64_t>(record.lowerBaseAddress) << 32
                        | (record.lowerLength / PageSize + 1);
                }
                else if (record.type == 2 && acpiLocation == 0 
                    && record.lowerBaseAddress >= 0x100'0000 && record.lowerBaseAddress < 0xf000'0000) {
                    possibleACPILocaton = static_cast<uint64_t>(record.lowerBaseAddress) << 32
                        | (record.lowerLength / PageSize + 1);
                }
            }
        }
        else 
        {
            freePage(0x8000, 1);
            totalPages++;
        }

        if (acpiLocation == 0) {
            acpiLocation = possibleACPILocaton;
        }

        return acpiLocation;
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
        kprintf("[PMM] Allocated: %d pages, %d KB; Free: %d pages, %d KB\n", 
            allocatedPages, allocatedPages * PageSize / 1024,
            freePages, freePages * PageSize / 1024);
    }

    uint32_t PhysicalMemoryManager::getFreePages() {
        return freePages;
    }

    uint32_t PhysicalMemoryManager::getTotalPages() {
        return totalPages;
    }

    #if TARGET_PREKERNEL
    void PhysicalMemoryManager::freePage(uintptr_t pageAddress, uint32_t count) {

        if (freePages == totalPages) {
            kprintf("[PMM] Tried to free an extra page: %x\n", pageAddress);
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

    #else

    void PhysicalMemoryManager::freePage(uintptr_t virtualAddress, uintptr_t physicalAddress) {
        auto page = static_cast<Page volatile*>(reinterpret_cast<void*>(virtualAddress & ~0xfff));
        page->nextFreePage = nextFreeAddress;
        nextFreeAddress = physicalAddress;
        allocatedPages--;
        freePages++;
    }

    #endif
}