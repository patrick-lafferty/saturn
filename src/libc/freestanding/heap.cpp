#include <heap.h>
#include <memory/physical_memory_manager.h>
#include <memory/virtual_memory_manager.h>
#include <stdio.h>

using namespace Memory;

namespace LibC_Implementation {

    Heap* KernelHeap {nullptr};

    const uint32_t kernelPageFlags = 
        static_cast<uint32_t>(PageTableFlags::AllowWrite);

    void Heap::initialize() {
        currentPage = currentVMM->allocatePages(1, kernelPageFlags);
        remainingPageSpace = PageSize;
    }

    void* Heap::aligned_allocate(size_t alignment, size_t size) {

        auto startingAddress = currentPage + (PageSize - remainingPageSpace);
        auto alignedAddress = (startingAddress + alignment - 1) & -alignment;

        if (startingAddress != alignedAddress) {
            auto padding = alignedAddress - startingAddress;

            allocate(padding);
        }

        return allocate(size);
    }

    void* Heap::allocate(size_t size) {

        auto startingAddress = currentPage + (PageSize - remainingPageSpace);

        if (size >= remainingPageSpace) {
            auto numberOfPages = size / PageSize + 1;
            
            while(size > PageSize) {
                size -= PageSize;
            }
            
            auto nextAddress = currentVMM->allocatePages(numberOfPages, kernelPageFlags);
            if (nextAddress - startingAddress > PageSize) {
                //printf("[Heap] Skipped address from %x to %x\n", startingAddress, nextAddress);
            }
            else {
                size -= remainingPageSpace;
            }

            remainingPageSpace = PageSize - size;

            currentPage = nextAddress + (numberOfPages - 1) * PageSize;
        }
        else {
            remainingPageSpace -= size;
        }

        return reinterpret_cast<void*>(startingAddress);
    }

    void createHeap() {
        auto virtualAddress = currentVMM->allocatePages(1, kernelPageFlags);
        auto physicalPage = Memory::currentPMM->allocatePage(1);
        Memory::currentVMM->map(virtualAddress, physicalPage);
        Memory::currentPMM->finishAllocation(virtualAddress, 1);

        KernelHeap = reinterpret_cast<Heap*>(virtualAddress);
        KernelHeap->initialize();
    }

}