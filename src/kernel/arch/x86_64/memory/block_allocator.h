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

namespace Memory {

    /*
    BlockAllocator is a kernel memory allocator that simplifies allocating
    instances of one type. Since the kernel doesn't have a heap, this
    is the main way kernel allocations are done.

    BlockAllocator keeps allocating new pages and only frees memory
    for its entire range at once using releaseMemory.
    */
    template<typename T>
    class BlockAllocator {
    public:

        BlockAllocator() {
            
            firstMetaBlock = allocateBlockMeta();
            currentMetaBlock = firstMetaBlock;
        }

        template<typename... Args>
        T* allocate(Args&&... args) {

            auto currentBlock = &currentMetaBlock->blocks[currentMetaBlock->currentBlock];

            if (currentBlock->remainingAllocations == 0) {
                currentMetaBlock->currentBlock++;

                if (currentMetaBlock->currentBlock < BlockMeta::MaxBlocks) {
                    currentMetaBlock->blocks[currentMetaBlock->currentBlock] = allocateBlock();
                }
                else {
                    auto meta = allocateBlockMeta();
                    currentMetaBlock->next = meta;
                    currentMetaBlock = meta;
                }

                currentBlock = &currentMetaBlock->blocks[currentMetaBlock->currentBlock];
            }

            auto ptr = currentBlock->buffer++;
            currentBlock->remainingAllocations--; 
            return new (ptr) T(std::forward<Args>(args)...);
        }

        T* allocateMultiple(int count) {

             /*
            Note: potentially wastes some memory since we're not starting at
            the current position in the buffer, but we can't guarantee that
            the next page address comes right after this one.
            */
            auto currentBlock = &currentMetaBlock->blocks[currentMetaBlock->currentBlock];

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
            return ptr;
        }

        void releaseMemory() {
            auto iterator = firstMetaBlock;
            auto& core = CPU::getCurrentCore();

            while (iterator != nullptr) {
                for (auto i = 0; i < iterator->currentBlock; i++) {
                    auto& block = iterator->blocks[i];
                    core.virtualMemory->freePages(block.blockStart, block.pagesRequired);
                }

                iterator = iterator->next;
            }
        }

    private:

        /*
        Each page can store 128 BlockMetas, with the last one being
        a pointer to a next page of 128 BlockMetas
        */
        struct Block {
            uintptr_t blockStart {0};
            uintptr_t pageCount {0};
            T* buffer;
            uint64_t remainingAllocations {0};
        };

        struct BlockMeta {
            Block blocks[120];
            BlockMeta* next {nullptr};
            int currentBlock {0};
            static const int MaxBlocks {120};
        };

        uintptr_t allocatePages(int pagesRequired) {
            auto& core = CPU::getCurrentCore();
            auto address = core.virtualMemory->allocatePages(pagesRequired, static_cast<uint32_t>(Memory::PageTableFlags::AllowWrite));

            for (auto i = 0; i < pagesRequired; i++) {
                auto virtualPage = address + Memory::PageSize * i;
                auto physicalPage = core.physicalMemory->allocatePage();
                core.virtualMemory->map(virtualPage, physicalPage);
                core.physicalMemory->finishAllocation(virtualPage);
            }

            return address;
        }

        Block allocateBlock(int numberOfTs) {

            auto size = sizeof(T) * numberOfTs;
            auto pagesRequired = (size / Memory::PageSize) + ((size % Memory::PageSize) > 0);
            auto address = allocatePages(pagesRequired);

            Block block;
            block.blockStart = address;
            block.pageCount = pagesRequired;
            block.remainingAllocations = numberOfTs;
            block.buffer = reinterpret_cast<T*>(address);

            return block;
        }

        Block allocateBlock() {
            auto tsPerPage = sizeof(T) / Memory::PageSize;
            return allocateBlock(tsPerPage);
        }

        BlockMeta* allocateBlockMeta() {
            auto meta = reinterpret_cast<BlockMeta*>(allocatePages(1));            
            meta->blocks[0] = allocateBlock();

            return meta;
        }

        BlockMeta* firstMetaBlock;
        BlockMeta* currentMetaBlock;
    };
}
