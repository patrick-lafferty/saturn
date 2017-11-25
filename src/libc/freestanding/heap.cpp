#include <heap.h>
#include <memory/physical_memory_manager.h>
#include <memory/virtual_memory_manager.h>
#include <stdio.h>

using namespace Memory;

namespace LibC_Implementation {

    Heap* KernelHeap {nullptr};

    const uint32_t kernelPageFlags = 
        static_cast<uint32_t>(PageTableFlags::AllowWrite);

    void Heap::initialize(uint32_t heapSize) {
        currentPage = currentVMM->allocatePages(heapSize / PageSize, kernelPageFlags);

        nextFreeChunk = reinterpret_cast<ChunkHeader*>(currentPage);
        nextFreeChunk->next = nullptr;
        nextFreeChunk->size = heapSize;
        nextFreeChunk->free = true;
        heapStart = nextFreeChunk;

        remainingPageSpace = PageSize - sizeof(ChunkHeader);
    }

    void* Heap::aligned_allocate(size_t alignment, size_t size) {

        /*auto startingAddress = currentPage + (PageSize - remainingPageSpace);
        auto alignedAddress = (startingAddress + alignment - 1) & -alignment;

        if (startingAddress != alignedAddress) {
            auto padding = alignedAddress - startingAddress;

            allocate(padding);
        }

        return allocate(size);*/

        auto chunk = findFreeChunk(size);
        auto startingAddress = reinterpret_cast<uint32_t>(chunk) + sizeof(ChunkHeader);
        auto alignedAddress = (startingAddress + alignment - 1) & -alignment;

        if (startingAddress != alignedAddress) {
            //can we fit it inside the chunk?
            auto chunkEndAddress = startingAddress + chunk->size;

            if ((alignedAddress + size) < chunkEndAddress) {
                if (alignedAddress - startingAddress < sizeof(ChunkHeader)) {
                    //TODO: not enough space to split chunk, wat do
                    printf("[HEAP] aligned_allocate not enough space in this chunk??\n");
                }
                else {
                    auto alignedChunk = reinterpret_cast<ChunkHeader*>(alignedAddress - sizeof(ChunkHeader));
                    alignedChunk->size = chunkEndAddress - alignedAddress;
                    alignedChunk->free = true;
                    alignedChunk->previous = chunk;
                    
                    if (chunk->next != nullptr) {
                        alignedChunk->next = chunk->next;
                    }

                    chunk->next = alignedChunk;

                    return allocate(alignedChunk, size);
                }
            }
            else {
                //TODO: need to find another chunk
                    printf("[HEAP] aligned_allocate need to find another chunk??\n");
            }
        }
        else {
            return allocate(chunk, size);
        }
    }

    ChunkHeader* Heap::findFreeChunk(size_t size) {
        if (nextFreeChunk->size >= size) {
            return nextFreeChunk;
        }
        else {
            auto chunk = heapStart;

            while (chunk != nullptr) {
                if (chunk->free && chunk->size >= size) {
                    return chunk;
                }                
            }
        }

        return nullptr;
    }

    void* Heap::allocate(ChunkHeader* chunk, size_t size) {
        chunk->free = false;
        
        //use up first part of chunk and split rest into new free
        //pages are already allocated
        auto currentAddress = reinterpret_cast<uint32_t>(chunk);
        currentAddress += sizeof(ChunkHeader);
        uint32_t allocatedAddress = allocatedAddress = currentAddress;
        currentAddress += size;

        if (chunk->size > size + sizeof(ChunkHeader)) {
            //theres space left over, create a new chunk with that space
            ChunkHeader* nextChunk = reinterpret_cast<ChunkHeader*>(currentAddress);
            nextChunk->size = chunk->size - size - sizeof(ChunkHeader);
            nextChunk->free = true;
            nextChunk->previous = chunk;

            if (chunk->next == nullptr) {
                chunk->next = nextChunk;
            }
            else {
                nextChunk->next = chunk->next;
                chunk->next = nextChunk;
            }

            chunk->size = size;

            nextFreeChunk = nextChunk;
        }

        return reinterpret_cast<void*>(allocatedAddress);
    }

    void* Heap::allocate(size_t size) {
       

        auto chunk = findFreeChunk(size);

        if (chunk == nullptr) {
            return nullptr;
        }

        return allocate(chunk, size); 
        /*auto startingAddress = currentPage + (PageSize - remainingPageSpace);

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

        return reinterpret_cast<void*>(startingAddress);*/
    }

    void Heap::free(void* ptr) {
        auto chunk = reinterpret_cast<ChunkHeader*>(reinterpret_cast<uint32_t>(ptr) - sizeof(ChunkHeader));
        chunk->free = true;

        if (chunk->previous != nullptr && chunk->previous->free) {
            chunk = combineChunkWithNext(chunk->previous);
        }

        if (chunk->next->free) {
            chunk = combineChunkWithNext(chunk);
        }

        //check if there are any allocated physical pages we can free
        auto startingAddress = reinterpret_cast<uint32_t>(chunk) + sizeof(ChunkHeader);
        auto alignedAddress = (startingAddress + PageSize - 1) & -PageSize;
        auto chunkEndAddress = startingAddress + chunk->size;

        if (alignedAddress < chunkEndAddress) {
            auto pagesToFree = (chunkEndAddress - alignedAddress) / PageSize;
            Memory::currentVMM->freePages(alignedAddress, pagesToFree);
        }
    }

    ChunkHeader* Heap::combineChunkWithNext(ChunkHeader* chunk) {
        chunk->size += chunk->next->size + sizeof(ChunkHeader);
        chunk->next = chunk->next->next;
        return chunk;
    }

    void Heap::HACK_syncPageWithVMM() {
        currentPage = currentVMM->allocatePages(1, kernelPageFlags);
        remainingPageSpace = PageSize;
    }

    void createHeap(uint32_t heapSize) {
        auto virtualAddress = currentVMM->allocatePages(1, kernelPageFlags);
        auto physicalPage = Memory::currentPMM->allocatePage(1);
        Memory::currentVMM->map(virtualAddress, physicalPage);
        Memory::currentPMM->finishAllocation(virtualAddress, 1);

        KernelHeap = reinterpret_cast<Heap*>(virtualAddress);
        KernelHeap->initialize(heapSize);
    }
}