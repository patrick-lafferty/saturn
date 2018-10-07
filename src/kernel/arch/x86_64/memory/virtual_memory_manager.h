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
        Invalid
    };

    class VirtualMemoryManager {
    public:

        static VirtualMemoryManager CreateFromLoader();

        VirtualMemoryManager();//PhysicalMemoryManager* physicalMemory);
        VirtualMemoryManager(VirtualMemoryManager const&) = delete;

        void map(uintptr_t virtualAddress, uintptr_t physicalAddress, uint32_t flags = 0);
        void unmap(uintptr_t virtualAddress, int count = 1);

        PageStatus getPageStatus(uintptr_t virtualAddress);

    };

}
