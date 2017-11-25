#include <stdlib.h>
#include <memory/physical_memory_manager.h>
#include <memory/virtual_memory_manager.h>
#include <heap.h>

using namespace Memory;
using namespace LibC_Implementation;
/*
Note: not thread-safe! Just want to get a simple version running
*/

void* malloc(size_t size) {
    
    if (KernelHeap == nullptr) {
        //error, we're not creating the heap here anymore
        //createHeap();
    }

    return KernelHeap->allocate(size);    
}