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
        nextFreeChunk->magic = 0xabababab;
        nextFreeChunk->magic2 = 0xcdcdcdcd;
        nextFreeChunk->size = heapSize - sizeof(ChunkHeader);
        nextFreeChunk->free = true;
        nextFreeChunk->next = nullptr;
        nextFreeChunk->previous = nullptr;
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

        auto chunk = findFreeAlignedChunk(size, alignment);
        if (chunk == nullptr) {
            return nullptr;
        }

        auto startingAddress = reinterpret_cast<uint32_t>(chunk) + sizeof(ChunkHeader);
        auto alignedAddress = (startingAddress + alignment - 1) & -alignment;

        if (startingAddress != alignedAddress) {
            //can we fit it inside the chunk?
            auto chunkEndAddress = startingAddress + chunk->size;

            //TODO: this should never be true
            if ((alignedAddress - startingAddress) < sizeof(ChunkHeader)) {
                //TODO: not enough space to split chunk, wat do
                printf("[HEAP] aligned_allocate not enough space in this chunk??\n");
            }
            else {
                auto alignedChunk = reinterpret_cast<ChunkHeader*>(alignedAddress - sizeof(ChunkHeader));
                alignedChunk->magic = 0xabababab;
                alignedChunk->magic2 = 0xcdcdcdcd;
                alignedChunk->size = chunkEndAddress - alignedAddress;// - sizeof(ChunkHeader);
                alignedChunk->free = true;
                alignedChunk->next = chunk->next;
                alignedChunk->previous = chunk;
                
                chunk->size = alignedAddress - startingAddress - sizeof(ChunkHeader);
                chunk->next = alignedChunk;

                return allocate(alignedChunk, size);
            }
        }
        else {
            return allocate(chunk, size);
        }
    }

    ChunkHeader* Heap::findFreeChunk(size_t size) {
        if (nextFreeChunk->size >= size && nextFreeChunk->free) {
            return nextFreeChunk;
        }
        else {
            auto chunk = heapStart;

            while (chunk != nullptr) {
                if (chunk->free && chunk->size >= size) {
                    return chunk;
                }                

                chunk = chunk->next;
            }
        }

        return nullptr;
    }

    ChunkHeader* Heap::findFreeAlignedChunk(size_t size, size_t alignment) {
        auto chunk = findFreeChunk(size);
        auto startingAddress = reinterpret_cast<uint32_t>(chunk) + sizeof(ChunkHeader);
        auto alignedAddress = (startingAddress + alignment - 1) & -alignment;

        if (startingAddress != alignedAddress) {
            //can we fit it inside the chunk?
            auto chunkEndAddress = startingAddress + chunk->size;

            if ((alignedAddress + size) < chunkEndAddress) {
                return chunk;
            }
            else {
                auto chunk = heapStart;

                while (chunk != nullptr) {
                    if (chunk != nextFreeChunk && chunk->size >= size) {
                        startingAddress = reinterpret_cast<uint32_t>(chunk) + sizeof(ChunkHeader); 
                        alignedAddress = (startingAddress + alignment - 1) & -alignment;

                        if (startingAddress != alignedAddress) {
                            //can we fit it inside the chunk?
                            auto chunkEndAddress = startingAddress + chunk->size;

                            if ((alignedAddress + size) < chunkEndAddress) {
                                return chunk;
                            }
                        }
                        else {
                            return chunk;
                        }
                    }

                    chunk = chunk->next;
                }
            }
        }

        printf("[HEAP] couldn't find free aligned chunk\n");
        return nullptr;
    }

    void* Heap::allocate(ChunkHeader* chunk, size_t size) {
        chunk->free = false;
        
        //use up first part of chunk and split rest into new free
        //pages are already allocated
        auto currentAddress = reinterpret_cast<uint32_t>(chunk);
        currentAddress += sizeof(ChunkHeader);
        uint32_t allocatedAddress = currentAddress;
        currentAddress += size;

        if (chunk->size > (size + sizeof(ChunkHeader))) {
            //theres space left over, create a new chunk with that space
            ChunkHeader* nextChunk = reinterpret_cast<ChunkHeader*>(currentAddress);
            nextChunk->magic = 0xabababab;
            nextChunk->magic2 = 0xcdcdcdcd;
            nextChunk->size = chunk->size - size - sizeof(ChunkHeader);
            nextChunk->free = true;
            nextChunk->next = chunk->next;
            nextChunk->previous = chunk;

            chunk->next = nextChunk;
            chunk->size = size;

            nextFreeChunk = nextChunk;
            /*printf("[Heap] nfc m: %x m2: %x s: %x n: %x p: %x\n", 
                nextFreeChunk->magic, nextFreeChunk->magic2,
                nextFreeChunk->size,
                nextFreeChunk->next, nextFreeChunk->previous);*/
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

        /*
        TODO: rethink this
        if (chunk->previous != nullptr && chunk->previous->free) {
            chunk = combineChunkWithNext(chunk->previous);
        }

        if (chunk->next != nullptr && chunk->next->free) {
            chunk = combineChunkWithNext(chunk);
        }*/

        //check if there are any allocated physical pages we can free
        auto startingAddress = reinterpret_cast<uint32_t>(chunk) + sizeof(ChunkHeader);
        auto alignedAddress = (startingAddress + PageSize - 1) & -PageSize;
        auto chunkEndAddress = startingAddress + chunk->size;

        if (alignedAddress < chunkEndAddress) {
            auto pagesToFree = (chunkEndAddress - alignedAddress) / PageSize;

            if (pagesToFree > 0) {
                Memory::currentVMM->freePages(alignedAddress, pagesToFree);
            }
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