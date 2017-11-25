#include <stdlib.h>
#include <memory/physical_memory_manager.h>
#include <memory/virtual_memory_manager.h>
#include <heap.h>

using namespace Memory;
using namespace LibC_Implementation;
/*
Note: not thread-safe! Just want to get a simple version running
*/

void free(void* ptr) {
    return KernelHeap->free(ptr);    
}