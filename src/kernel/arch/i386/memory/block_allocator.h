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

namespace Kernel {

    /*
    Temp design just to make sure everything else works first, no freeing
    */
    template<typename T>
    class BlockAllocator {
    public:

        BlockAllocator(Memory::VirtualMemoryManager* vmm, int maxSize = 0)
            : vmm {vmm}, maxSize {maxSize} {

            if (maxSize > 1) {
                getBlocks(maxSize);
            }
        }

        T* allocate() {
            if (blocksRemaining == 0) {
                getBlocks(maxSize > 0 ? maxSize : 10);
            }

            blocksRemaining--;

            auto ptr = buffer;
            buffer++;
            return new (ptr) T;
        }

        T* allocateFrom(uint32_t address) {
            vmm->HACK_setNextAddress(address);
            return allocate();
        }

        T* allocateMultiple(int count) {

            if (count > blocksRemaining) {
                getBlocks(count);
            }

            blocksRemaining -= count;
            auto ptr = buffer;
            buffer += count;
            return ptr;
        }

        void free(T* ptr) {

        }

    private:

        void getBlocks(int count) {
            auto size = sizeof(T) * count;
            auto pagesRequired = (size / Memory::PageSize) + ((size % Memory::PageSize) > 0);
            blocksRemaining = count;
            auto address = vmm->allocatePages(pagesRequired, static_cast<uint32_t>(Memory::PageTableFlags::AllowWrite));

            for (auto i = 0u; i < pagesRequired; i++) {
                auto virtualPage = address + Memory::PageSize * i;
                auto physicalPage = Memory::currentPMM->allocatePage(1);
                vmm->map(virtualPage, physicalPage);
                Memory::currentPMM->finishAllocation(virtualPage, 1);
            }

            buffer = reinterpret_cast<T*>(address);
        }

        Memory::VirtualMemoryManager* vmm;
        int blocksRemaining {0};
        int maxSize {0};
        T* buffer;
    };
}
