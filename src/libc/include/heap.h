#pragma once

#include <stddef.h>
#include <stdint.h>

namespace LibC_Implementation {

    class Heap {
    public:

        void initialize();
        void* allocate(size_t size);
        void* aligned_allocate(size_t alignment, size_t size);

    private:
        
        uint32_t currentPage {0};
        uint32_t remainingPageSpace {0};
    };

    void createHeap();

    extern Heap* KernelHeap;
}