#include <stdlib.h>
#include <memory/physical_memory_manager.h>
#include <memory/virtual_memory_manager.h>

using namespace Memory;
/*
Note: not thread-safe! Just want to get a simple version running
*/
void* currentPage {nullptr};
uint32_t remainingPageSpace {0};

const uint32_t kernelPageFlags = 
    static_cast<uint32_t>(PageTableFlags::Present)
    | static_cast<uint32_t>(PageTableFlags::AllowWrite);

void* malloc(size_t size) {
    if (currentPage == nullptr) {
        currentPage = reinterpret_cast<void*>(currentVMM->allocatePages(1, kernelPageFlags));

        remainingPageSpace = PageSize;
    }

    auto startingAddress = currentPage;
    currentPage = static_cast<void*>(static_cast<uint8_t*>(currentPage) + size);

    if (size > remainingPageSpace) {
        size -= remainingPageSpace;
        auto numberOfPages = size / PageSize + 1;
        remainingPageSpace = numberOfPages * PageSize - size;

        currentVMM->allocatePages(numberOfPages, kernelPageFlags);
    }
    else {
        remainingPageSpace -= size;
    }

    return currentPage;
}