/*
Copyright (c) 2018, Patrick Lafferty
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

#include "virtual_memory_manager.h"
#include "physical_memory_manager.h"
#include <cpu/metablocks.h>
#include <utility>
#include "address_space.h"

namespace Memory {

    /*
    BlockAllocator is a kernel memory allocator that simplifies allocating
    instances of one type. Since the kernel doesn't have a heap, this
    is the main way kernel allocations are done.

    BlockAllocator keeps allocating new pages and only frees memory
    for its entire range at once using releaseMemory, but it can 
    reuse memory for previously freed allocations.
    */
    template<typename T>
    class BlockAllocator {
    public:

        BlockAllocator(int initialElementCount)
            : elementsPerBlock {initialElementCount} {
            
            initialBlock = createBlock(initialElementCount * 10);
            currentBlock = initialBlock;
        }

        template<typename... Args>
        T* allocate(Args&&... args) {

            if (freeList != nullptr) {
                auto allocation = freeList;
                freeList = allocation->nextFree;
                return new (&allocation->value) T(std::forward<Args>(args)...);
            }

            if (currentBlock->remainingAllocations == 0) {
                auto block = createBlock(elementsPerBlock);

                if (block == nullptr) {
                    return nullptr;
                }

                currentBlock->next = block;
                currentBlock = block;
            }

            auto ptr = currentBlock->buffer++;
            currentBlock->remainingAllocations--;
            return new (ptr) T(std::forward<Args>(args)...);
        }

        T* allocateMultiple(int count) {

            //TODO: this should be an ArrayAllocator or SequentialAllocator

             /*
            Note: potentially wastes some memory since we're not starting at
            the current position in the buffer, but we can't guarantee that
            the next page address comes right after this one.
            */
            /*auto currentBlock = &currentMetaBlock->blocks[currentMetaBlock->currentBlock];

            if (currentBlock->remainingAllocations < count) {
                if (currentBlock->currentBlock < BlockMeta::MaxBlocks) {
                   currentMetaBlock->blocks[currentMetaBlock->currentBlock] = allocateBlock();
                }
                else {
                    auto meta = allocateBlockMeta();
                    currentMetaBlock->next = meta;
                    currentMetaBlock = meta;
                }

                currentBlock = &currentMetaBlock->blocks[currentMetaBlock->currentBlock];
            }

            auto ptr = currentBlock->buffer;
            currentBlock->buffer += count;
            currentBlock->remainingAllocations -= count;
            return ptr;*/
        }

        void free(T* ptr) {
            auto address = reinterpret_cast<uintptr_t>(ptr);
            address -= sizeof(uintptr_t);
            auto allocation = reinterpret_cast<Allocation*>(address);

            if (freeList != nullptr) {
                allocation->nextFree = freeList;
                freeList = allocation;
            }
            else {
                freeList = allocation;
            }
        }

        void releaseMemory() {

            auto iterator = initialBlock;
            auto& core = CPU::getCurrentCore();

            while (iterator != nullptr) {
                core.addressSpace->release(iterator->reservation);
                iterator = iterator->next;
            }
        }

    private:

        struct Allocation {
            Allocation* nextFree;
            T value;
        };

        Allocation* freeList {nullptr};

        struct Block {
            Block(Allocation* buffer, int allocations, AddressReservation reservation)
                : buffer {buffer}, 
                    remainingAllocations {allocations},
                    reservation {reservation} {
            }

            Allocation* buffer {nullptr};
            int remainingAllocations {0};
            AddressReservation reservation;
            Block* next {nullptr};
        };

        Block* createBlock(int numberOfElements) {
            auto& core = CPU::getCurrentCore(); 
            auto requiredSize = sizeof(Allocation) * numberOfElements;
            requiredSize += sizeof(Block);
            requiredSize = (requiredSize & ~0xFFF) + 0x1000;

            auto maybeReservation = core.addressSpace->reserve(requiredSize);
            if (!maybeReservation.has_value()) {
                return nullptr;
            }

            auto reservation = maybeReservation.value();
            auto maybeAllocation = reservation.allocatePages(requiredSize / 0x1000);
            
            if (!maybeAllocation.has_value()) {
                return nullptr;
            }

            auto allocation = maybeAllocation.value();

            uint8_t* ptr = reinterpret_cast<uint8_t*>(allocation);
            auto block = new (ptr) Block(
                reinterpret_cast<Allocation*>(ptr + sizeof(Block)),
                numberOfElements,
                reservation);
            
            return block;
        }

        Block* initialBlock;
        Block* currentBlock;

        int elementsPerBlock;
         
    };
}
