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
#include <optional>
#include "addresses.h"

namespace Kernel {
    struct Task;
}

extern "C" void activateVMM(Kernel::Task*);

namespace Memory {

    class PhysicalMemoryManager;

    struct PageMapLevel4 {
        uintptr_t directoryPointers[512];
    };
    
    struct PageDirectoryPointer {
        uintptr_t directoryTables[512];
    };

    struct PageDirectoryTable {
        uintptr_t pageTables[512];
    };

    struct PageTable {
        uintptr_t pages[512];
    };

    enum class PageTableFlags {
        Present = 1 << 0,
        AllowWrite = 1 << 1,
        AllowUserModeAccess = 1 << 2,
        WriteThrough = 1 << 3,
        CacheDisable = 1 << 4,
        Accessed = 1 << 5,
        Dirty = 1 << 6
    };

    enum class PageStatus {
        Allocated,
        Mapped,
        UnallocatedPageTable,
        Invalid
    };

    class VirtualMemoryManager {
    public:

        static VirtualMemoryManager CreateFromLoader();

        VirtualMemoryManager() = default;
        VirtualMemoryManager(VirtualMemoryManager const&) = delete;

        /*
        Maps virtualAddress to physicalAddress so that any reads/writes
        to virtualAddress actually happen in RAM at physicalAddress
        */
        void map(VirtualAddress virtualAddress, PhysicalAddress physicalAddress, uint32_t flags = 0);

        /*
        Removes the mapping. This does not free the physical page.
        */
        void unmap(VirtualAddress virtualAddress);

        /*
        Ensures that all of the appropriate paging structures are
        setup for this virtual address. Returns the physical address
        of the page if it was allocated in this func
        */
        PhysicalAddress allocatePagingTablesFor(VirtualAddress virtualAddress, PhysicalMemoryManager* pmm);

        /*
        Copies the initial 2MB identity map and the kernel PDP
        and sets up a new paging hierarchy.

        Returns the physical address of the level 4 page map.
        */
        std::optional<PhysicalAddress> clone();

        PageStatus getPageStatus(VirtualAddress virtualAddress);
    };
}