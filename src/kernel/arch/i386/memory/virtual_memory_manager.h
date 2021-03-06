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

/*
Two instances of this file are compiled to two separate objs, one
to be linked at a low address to be used for the prekernel before paging,
and one to be used by the real kernel after paging is setup. So there isn't
any duplicate symbols, have different namespaces for each obj.
*/

namespace Kernel {
    struct Task;
}

extern "C" void activateVMM(Kernel::Task*);


#if TARGET_PREKERNEL
namespace MemoryPrekernel {
#else
namespace Memory {
#endif

    const uint32_t KernelVirtualStartingAddress = 0xd000'0000;//0xCFFF'e000;

    class PhysicalMemoryManager;

    struct PageDirectory {
        uintptr_t pageTableAddresses[1024];
    };

    struct PageTable {
        uintptr_t pageAddresses[1024];
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

        void cleanup();
        void cleanupClonePageTables(PageDirectory& cloneDirectory, uint32_t kernelStackAddress);
        PageDirectory getPageDirectory();

        /*
        Only called once, when kernel_main starts. The pre-kernel setup
        creates a PhysicalMemoryManager at a low address, and once
        we jump to the higher half kernel we create a brand new one
        with its values copied. Same thing for the VirtualMemoryManager,
        once it gets copied it has a pointer to the low address PMM,
        hence why we replace it with this.
        */
        void setPhysicalManager(PhysicalMemoryManager* physical);

        /*
        Normally this would be a constructor, but:
        1) we need a static instance declared for the prekernel to reserve
            space for it, and the actual construction has to wait until setupKernel()
        2) we don't have an allocator setup before the VMM is setup, so we couldn't
            just replace 1) with a pointer instead of a value and = new instead of 
            initialize
        */
        void initialize(PhysicalMemoryManager* physical);

        /*
        ensures that the appropriate page table exists, and creates one
        if necessary
        */
        uintptr_t allocatePages(uint32_t count, uint32_t flags = 0, uintptr_t startingVirtualAddress = 0);

        /*
        must only be called before paging is enabled
        to enforce that, make use of the fact that two VirtualMemoryManager objects
        will be compiled, one for the prekernel and one for the real one, so only
        compile this function for the prekernel version
        */
        #if TARGET_PREKERNEL
        void map_unpaged(uintptr_t virtualAddress, uintptr_t physicalAddress, uint32_t pageCount, uint32_t flags);
        #endif

        /*
        maps the virtual address to the physical address

        note: the page table for the address must already exist, so
        allocatePages should be called before map
        */
        void map(uintptr_t virtualAddress, uintptr_t physicalAddress, uint32_t flags = 0, bool updateCR3 = true);

        /*
        unmaps the virtual address

        TODO - should this also free the physical address?
        */
        void unmap(uintptr_t virtualAddress, uint32_t count);

        void remap(uintptr_t virtualAddress, uintptr_t newAddress);

        void freePages(uintptr_t virtualAddress, uint32_t count);

        /*
        enables paging and sets this VMM's directory as the current one
        reassigns directory to use the virtual address instead of physical
        */
        void activate();

        PageStatus getPageStatus(uintptr_t virtualAddress);

        void HACK_setNextAddress(uint32_t address);
        uint32_t HACK_getNextAddress() {
            return nextAddress;
        }

        VirtualMemoryManager* cloneForUsermode(VirtualMemoryManager* vmm, bool copyAll = false);

        void preallocateKernelPageTables();

        void sharePages(uint32_t ownerStartAddress, VirtualMemoryManager* recipientVMM, uint32_t recipientStartAddress, uint32_t count);

        uint32_t getDirectoryPhysicalAddress() const {
            return directoryPhysicalAddress;
        }

    private:

        uintptr_t allocatePageTable(uintptr_t virtualAddress, int index);

        PhysicalMemoryManager* physicalManager;
        PageDirectory* directory;
        uint32_t directoryPhysicalAddress;
        uintptr_t nextAddress {0};
        bool pagingActive {false};
    };

    extern VirtualMemoryManager* InitialKernelVMM;
    #if not TARGET_PREKERNEL
    VirtualMemoryManager* getCurrentVMM();
    #endif
}
