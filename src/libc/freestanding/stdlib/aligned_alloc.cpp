#include <stdlib.h>
#include <memory/virtual_memory_manager.h>
#include <heap.h>

using namespace Memory;
using namespace LibC_Implementation;
/*
Note: not thread-safe! Just want to get a simple version running
*/

void* aligned_alloc(size_t alignment, size_t size) {
    if (KernelHeap == nullptr) {
        //error, we're not creating the heap here anymore
        //createHeap();
    }

    return KernelHeap->aligned_allocate(alignment, size);
}