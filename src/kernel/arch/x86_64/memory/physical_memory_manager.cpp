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

    void PhysicalMemoryManager::SetupGlobalManager(uint64_t firstFreeAddress, uint64_t totalFreePages) {
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

        return nextFreeAddress;
    }

    void PhysicalMemoryManager::finishAllocation(uintptr_t pageAddress) {
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
}
