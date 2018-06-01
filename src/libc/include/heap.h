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

#include <stddef.h>
#include <stdint.h>

namespace Memory {
    class VirtualMemoryManager;
}

namespace LibC_Implementation {

    struct ChunkHeader {
        uint32_t magic {0xabababab};
        uint32_t magic2 {0xcdcdcdcd};
        ChunkHeader* next;
        ChunkHeader* previous; 
        uint32_t size : 31;
        bool free : 1;
    };

    struct FixedSizeHeader {
        FixedSizeHeader* next;
        uint32_t size: 29;
        bool free : 1;
        int unused : 2;
    };

    struct RegularHeader {
        uint32_t size: 29;
        bool free : 1;
        int unused : 2;
        RegularHeader* next;
        RegularHeader* prev;
    };

    class Heap {
    public:

        void initialize(uint32_t heapSize, Memory::VirtualMemoryManager* vmm);
        void* allocate(size_t size);
        void* aligned_allocate(size_t alignment, size_t size);
        void free(void* ptr);
        void HACK_syncPageWithVMM();

    private:

        void* allocate(ChunkHeader* chunk, size_t size);
        ChunkHeader* findFreeChunk(size_t size);
        ChunkHeader* findFreeAlignedChunk(size_t size, size_t alignment);
        ChunkHeader* combineChunkWithNext(ChunkHeader* chunk);
        
        uint32_t currentPage {0};
        uint32_t remainingPageSpace {0};
        ChunkHeader* heapStart;
        ChunkHeader* nextFreeChunk;

        void* allocateFromFixed(size_t size);
        FixedSizeHeader* preallocateFixedChunks(int size, int count);

        static const int MaxNumberOfBins {32};
        static const int MaxFixedBinSize {256};

        FixedSizeHeader* fixedBins[MaxNumberOfBins];

        int filledFromFixed {0};
        int filledFromOld {0};

        int alignedFilledFromFixed {0};
        int possibleFixedAligned {0};
        int alignedFilledFromOld {0};

        int freeFromFixed {0};
        int freeFromOld {0};

        int preallocatedTimesCalled {0};
    };

    void createHeap(uint32_t heapSize, Memory::VirtualMemoryManager* vmm);

    extern Heap* KernelHeap;
}