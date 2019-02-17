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
#include <string.h>

namespace Memory {

    PhysicalMemoryManager PhysicalMemoryManager::GlobalManager;
    uint64_t PhysicalMemoryManager::DMAFreePages[512];

    void PhysicalMemoryManager::SetupGlobalManager(uint64_t firstFreeAddress, uint64_t totalFreePages) {

        memset(DMAFreePages, 0xFFFF'FFFF, sizeof(DMAFreePages));

        GlobalManager.nextFreeAddress = firstFreeAddress;
        GlobalManager.totalPages = totalFreePages;
        GlobalManager.freePages = totalFreePages;
    }

    PhysicalMemoryManager& PhysicalMemoryManager::GetGlobalManager() {
        return GlobalManager;
    }

    uintptr_t PhysicalMemoryManager::allocatePage() {
        /*
        TODO:

        each physical core gets its own PMM
        the global PMM (GPMM) hands out batches of
        pages to core PMMs (CPMM). When a CPMM is empty,
        it requests pages from the GPMM, needs
        to support locks there.
        */
        if (freePages == 0) {
            return 0;
        }

        freePages--;

        if (waitingToFinish) {
            int pause = 0;
        }

        waitingToFinish = true;

        return nextFreeAddress;
    }

    void PhysicalMemoryManager::finishAllocation(uintptr_t pageAddress) {
        waitingToFinish = false;
        uint64_t* page = reinterpret_cast<uint64_t*>(pageAddress & ~0xFFF);
        nextFreeAddress = *page;
        memset(page, 0, PageSize);
    }

    uint32_t PhysicalMemoryManager::getFreePages() {
        return freePages;
    }

    uint32_t PhysicalMemoryManager::getTotalPages() {
        return totalPages;
    }

    void PhysicalMemoryManager::freePage(uintptr_t virtualAddress, uintptr_t physicalAddress) {
        auto page = reinterpret_cast<uint64_t*>(virtualAddress & ~0xfff);
        *page = nextFreeAddress;
        nextFreeAddress = physicalAddress;
        freePages++;
    }

    uintptr_t PhysicalMemoryManager::allocateDMAPage() {
        //TODO: locks

        const int firstMegabyte = 0x100'000;

        for (int i = 3; i < 512; i++) {
            if (DMAFreePages[i] > 0) {
                uint32_t index {0};

                asm("bsf %1, %0"
                    : "=r" (index)
                    : "rm" (DMAFreePages[i]));

                DMAFreePages[i] &= ~(1 << index);

                auto page = 64 * PageSize * i;
                page += index * PageSize;

                return page + firstMegabyte;
            }
        }

        return 0;
    }

    void PhysicalMemoryManager::freeDMAPage(uintptr_t address) {
        const int firstMegabyte = 0x100'000;

        if (address < firstMegabyte) {
            return;
        }

        address -= firstMegabyte;
        int index = address / (64 * 0x1000);
        int bit = (address % (64 * 0x1000)) / PageSize;

        if (index >= 512) {
            return;
        }

        DMAFreePages[index] |= (1 << bit);
    }
}
