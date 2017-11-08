#include "virtual_memory_manager.h"
#include "physical_memory_manager.h"
#include <string.h>
#include <stdio.h>

namespace Memory {

    VirtualMemoryManager* currentVMM;

    inline uint32_t extractDirectoryIndex(uintptr_t virtualAddress) {
        return virtualAddress >> 22;
    }

    inline uint32_t extractTableIndex(uintptr_t virtualAddress) {
        return (virtualAddress >> 12) & 0x3FF;
    }

    inline uintptr_t calculatePageTableAddress(uintptr_t virtualAddress) {
        auto directoryIndex = extractDirectoryIndex(virtualAddress);
        return 0xFFC00000 + PageSize * directoryIndex;
    }

    VirtualMemoryManager::VirtualMemoryManager(PhysicalMemoryManager& physical)
        : physicalManager{physical} 
        {

        auto page = physical.allocatePage(1);
        physical.finishAllocation(page, 1);
        directory = static_cast<PageDirectory*>(reinterpret_cast<void*>(page));
        directory->pageTableAddresses[1023] = reinterpret_cast<uintptr_t>(static_cast<void*>(directory)) | 7;//3;
    }

    void updateCR3Address(PageDirectory* directory) {
        auto directoryAddress = directory->pageTableAddresses[1023] & ~0x3ff;
        
        asm("movl %%eax, %%CR3 \n"
            : //no output
            : "a" (directoryAddress)
        );
    }

    uintptr_t VirtualMemoryManager::allocatePageTable(uintptr_t virtualAddress, int index) {
        auto physicalPage = physicalManager.allocatePage(1);

        directory->pageTableAddresses[index] = physicalPage | 7;//3;

        updateCR3Address(directory);

        if (pagingActive) {
            physicalManager.finishAllocation(virtualAddress, 1);
        }
        else {
            physicalManager.finishAllocation(physicalPage, 1);
        }

        return physicalPage;
    }

    uintptr_t VirtualMemoryManager::allocatePages(uint32_t count, uint32_t flags, uintptr_t startingVirtualAddress) {
        /*
        mark nextAddress -> nextAddress + PageSize * count in the directory
        */
        //TODO: use avl tree to assign virtual address
        uintptr_t startingAddress, virtualAddress;

        if (startingVirtualAddress != 0) {
            startingAddress = startingVirtualAddress;
            virtualAddress = startingVirtualAddress;
        }
        else {
            startingAddress = nextAddress;
            virtualAddress = nextAddress;
        }

        nextAddress += PageSize * count;

        for(uint32_t i = 0; i < count; i++) {
            auto directoryIndex = extractDirectoryIndex(virtualAddress);

            if (directory->pageTableAddresses[directoryIndex] == 0) {
                if (pagingActive) {
                    allocatePageTable(0xFFC00000 + PageSize * directoryIndex, directoryIndex);
                }
                else {
                    allocatePageTable(nextAddress, directoryIndex);
                }

                nextAddress += PageSize;
            }

            if (pagingActive) {
                auto pageTableAddress = 0xFFC00000 + PageSize * directoryIndex;
                auto pageTable = static_cast<PageTable*>(reinterpret_cast<void*>(pageTableAddress));

                auto tableIndex = extractTableIndex(virtualAddress);
                pageTable->pageAddresses[tableIndex] = flags; 
            }

            virtualAddress += PageSize;
        }

        //TODO: add dirty flag to check if this is necessary
        updateCR3Address(directory);

        return startingAddress;
    }

    void VirtualMemoryManager::map_unpaged(uintptr_t virtualAddress, uintptr_t physicalAddress, uint32_t pageCount, uint32_t flags) {
        
        allocatePages(pageCount, flags, virtualAddress);

        for (auto i = 0u; i < pageCount; i++) {
            auto directoryIndex = extractDirectoryIndex(virtualAddress);
            auto pageTableAddress = directory->pageTableAddresses[directoryIndex] & 0xFFFFF000;
            auto pageTable = static_cast<PageTable*>(reinterpret_cast<void*>(pageTableAddress));
            
            auto tableIndex = extractTableIndex(virtualAddress);
            pageTable->pageAddresses[tableIndex] = physicalAddress | flags;

            virtualAddress += PageSize;
            physicalAddress += PageSize;
        }

        if (virtualAddress > nextAddress) {
            nextAddress = virtualAddress;
        }
    }

    void VirtualMemoryManager::map(uintptr_t virtualAddress, uintptr_t physicalAddress, uint32_t flags) {
        
        auto pageTableAddress = calculatePageTableAddress(virtualAddress);
        auto pageTable = static_cast<PageTable*>(reinterpret_cast<void*>(pageTableAddress));
        auto tableIndex = extractTableIndex(virtualAddress);

        if (flags == 0) {
            flags = pageTable->pageAddresses[tableIndex] & 0xFF; 
        }

        flags |= static_cast<uint32_t>(PageTableFlags::Present);

        pageTable->pageAddresses[tableIndex] = physicalAddress | flags; 
        updateCR3Address(directory);
    }

    PageStatus VirtualMemoryManager::getPageStatus(uintptr_t virtualAddress) {
        
        auto pageTableAddress = calculatePageTableAddress(virtualAddress);
        auto pageTable = static_cast<PageTable*>(reinterpret_cast<void*>(pageTableAddress));
        auto tableIndex = extractTableIndex(virtualAddress);       
        auto physicalAddress = pageTable->pageAddresses[tableIndex];

        if (physicalAddress & 0xFF) {
            if (physicalAddress & ~0xFF) {
                return PageStatus::Mapped;
            }
            else {
                return PageStatus::Allocated;
            }
        }
        else {
            return PageStatus::Invalid;
        }
    }

    void VirtualMemoryManager::HACK_setNextAddress(uint32_t address)
    {
        nextAddress = address;
    }

    void VirtualMemoryManager::activate() {
        auto directoryAddress = reinterpret_cast<uintptr_t>(static_cast<void*>(directory));

        asm("movl %%eax, %%CR3 \n"
            "movl %%CR0, %%eax \n"
            "or $0x80000001, %%eax \n"
            "movl %%eax, %%CR0 \n"
            : //no output
            : "a" (directoryAddress)
        );

        directoryAddress = 0xFFFFF000;
        directory = static_cast<PageDirectory*>(reinterpret_cast<void*>(directoryAddress));
        pagingActive = true;
        currentVMM = this;
    }

}