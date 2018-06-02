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

    //TODO: HACK: decide how ISR can get access to current PMM
    extern PhysicalMemoryManager* currentPMM;
}
