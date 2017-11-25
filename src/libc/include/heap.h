#pragma once

#include <stddef.h>
#include <stdint.h>

namespace LibC_Implementation {

    struct ChunkHeader {
        uint32_t size : 31;
        bool free : 1;
        ChunkHeader* next;
        ChunkHeader* previous; 
    };

    class Heap {
    public:

        void initialize(uint32_t heapSize);
        void* allocate(size_t size);
        void* aligned_allocate(size_t alignment, size_t size);
        void free(void* ptr);
        void HACK_syncPageWithVMM();

    private:

        void* allocate(ChunkHeader* chunk, size_t size);
        ChunkHeader* findFreeChunk(size_t size);
        ChunkHeader* combineChunkWithNext(ChunkHeader* chunk);
        
        uint32_t currentPage {0};
        uint32_t remainingPageSpace {0};
        ChunkHeader* heapStart;
        ChunkHeader* nextFreeChunk;
    };

    void createHeap(uint32_t heapSize);

    extern Heap* KernelHeap;
}