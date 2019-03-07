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
#pragma once

#include <stdint.h>

namespace Memory {

    const int PageSize {0x1000};

    class PhysicalMemoryManager {
    public:

        static void SetupGlobalManager(uint64_t firstFreeAddress, uint64_t totalFreePages);
        static PhysicalMemoryManager& GetGlobalManager();

        /*
        returns a physical address to a free page, and tracks stats.
        we can't get the address of the next free page yet,
        because its written in the page itself which needs to be first
        page mapped before it can be read. so, each call to 
        allocatePage() needs to be followed by finishAllocation()
        */
        [[nodiscard]]
        uintptr_t allocatePage();

        /*
        here pageAddress is the virtual address mapped from the physical
        address of the previous call to allocatePage. now we can read the
        page and get the physical address of the next page
        */
        void finishAllocation(uintptr_t pageAddress);
        void freePage(uintptr_t virtualAddress, uintptr_t physicalAddress);

        uint32_t getFreePages();
        uint32_t getTotalPages();

        /*
        The first 640kb is reserved for the rare times where we're
        stuck in real mode, such as AP trampolines
        */
        [[nodiscard]]
        static uintptr_t allocateRealModePage();
        static void freeRealModePage(uintptr_t address);

        /*
        Separate range for 1MB - 16MB, for DMA
        */
        [[nodiscard]]
        static uintptr_t allocateDMAPage();
        static void freeDMAPage(uintptr_t address);

    private:

        uintptr_t nextFreeAddress {0};
        uint64_t totalPages {0};
        uint64_t freePages {0};

        bool waitingToFinish {false};

        static PhysicalMemoryManager GlobalManager;

        //640kb ought to be enough for anyone
        static uint64_t RealModeFreePages [20];
        static uint64_t DMAFreePages [512];
    };
}
