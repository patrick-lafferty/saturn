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
#include <heap.h>
#include <memory/physical_memory_manager.h>
#include <memory/virtual_memory_manager.h>
#include <stdio.h>
#include <new>

using namespace Memory;

namespace LibC_Implementation {

    Heap* KernelHeap {nullptr};

    const uint32_t kernelPageFlags = 
        static_cast<uint32_t>(PageTableFlags::AllowWrite);

    void Heap::initialize(uint32_t heapSize, Memory::VirtualMemoryManager* vmm) {
        currentPage = vmm->allocatePages(heapSize / PageSize, kernelPageFlags);

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

    void uhoh() {
        while (true) {}
    }

    void* Heap::aligned_allocate(size_t alignment, size_t size) {

        auto chunk = findFreeAlignedChunk(size, alignment);
        if (chunk == nullptr) {
            uhoh();
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
                uhoh();
                kprintf("[HEAP] aligned_allocate not enough space in this chunk??\n");
                return nullptr;
            }
            else {
                auto alignedChunk = reinterpret_cast<ChunkHeader*>(alignedAddress - sizeof(ChunkHeader));
                alignedChunk->magic = 0xabababab;
                alignedChunk->magic2 = 0xcdcdcdcd;
                alignedChunk->size = chunkEndAddress - alignedAddress;
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

        uhoh();

        kprintf("[Heap] findFreeChunk for %d size returned null\n", size);

        return nullptr;
    }

    ChunkHeader* Heap::findFreeAlignedChunk(size_t size, size_t alignment) {
        auto chunk = findFreeChunk(size);
        auto startingAddress = reinterpret_cast<uint32_t>(chunk) + sizeof(ChunkHeader);
        auto alignedAddress = (startingAddress + alignment - 1) & -alignment;

        if (startingAddress != alignedAddress) {
            //can we fit it inside the chunk?
            auto chunkEndAddress = startingAddress + chunk->size;

            if ((alignedAddress + size) <= chunkEndAddress) {
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

        uhoh();

        kprintf("[HEAP] couldn't find free aligned chunk\n");
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
        }
        else {
            uhoh();
        }

        return reinterpret_cast<void*>(allocatedAddress);
    }

    void* Heap::allocate(size_t size) {
       

        auto chunk = findFreeChunk(size);

        if (chunk == nullptr) {
            uhoh();
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

        return;

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
        currentPage = Memory::currentVMM->allocatePages(1, kernelPageFlags);
        remainingPageSpace = PageSize;
    }

    void createHeap(uint32_t heapSize, Memory::VirtualMemoryManager* vmm) {
        auto virtualAddress = vmm->allocatePages(1, kernelPageFlags);
        auto physicalPage = Memory::currentPMM->allocatePage(1);
        vmm->map(virtualAddress, physicalPage);
        Memory::currentPMM->finishAllocation(virtualAddress, 1);
        auto ptr = reinterpret_cast<void*>(virtualAddress);

        KernelHeap = new (ptr) Heap;
        KernelHeap->initialize(heapSize, vmm);
    }
}