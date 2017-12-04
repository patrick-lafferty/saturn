#pragma once

#include <stdint.h>

/*
Two instances of this file are compiled to two separate objs, one
to be linked at a low address to be used for the prekernel before paging,
and one to be used by the real kernel after paging is setup. So there isn't
any duplicate symbols, have different namespaces for each obj.
*/

namespace Memory {
    class VirtualMemoryManager;
}

extern "C" void activateVMM(Memory::VirtualMemoryManager*);


#if TARGET_PREKERNEL
namespace MemoryPrekernel {
#else
namespace Memory {
#endif

    const uint32_t KernelVirtualStartingAddress = 0xCFFF'F000;

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

    //TODO: HACK: decide on how ISR can get access to current VMM
    extern class VirtualMemoryManager* currentVMM;

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
        void map(uintptr_t virtualAddress, uintptr_t physicalAddress, uint32_t flags = 0);

        /*
        unmaps the virtual address

        TODO - should this also free the physical address?
        */
        void unmap(uintptr_t virtualAddress, uint32_t count);

        void freePages(uintptr_t virtualAddress, uint32_t count);

        /*
        enables paging and sets this VMM's directory as the current one
        reassigns directory to use the virtual address instead of physical
        */
        void activate();

        PageStatus getPageStatus(uintptr_t virtualAddress);

        void HACK_setNextAddress(uint32_t address);

        VirtualMemoryManager* cloneForUsermode();

    private:

        uintptr_t allocatePageTable(uintptr_t virtualAddress, int index);

        PhysicalMemoryManager* physicalManager;
        PageDirectory* directory;
        uint32_t directoryPhysicalAddress;
        uintptr_t nextAddress {0};
        bool pagingActive {false};
    };
}