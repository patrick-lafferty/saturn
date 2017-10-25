#include "virtual_memory_manager.h"
#include "physical_memory_manager.h"
#include <string.h>
#include <stdio.h>

namespace Memory {

    VirtualMemoryManager::VirtualMemoryManager(PhysicalMemoryManager& physical)
        : physicalManager{physical} 
        {

        auto page = physical.allocatePage(1);
        physical.finishAllocation(page, 1);
        directory = static_cast<PageDirectory*>(reinterpret_cast<void*>(page));

        memset(directory->pageTableAddresses, 0, sizeof(directory->pageTableAddresses));

        page = physical.allocatePage(1);
        physical.finishAllocation(page, 1);
        PageTable* table = static_cast<PageTable*>(reinterpret_cast<void*>(page));
        memset(table, 0, PageSize);

        directory->pageTableAddresses[0] = page | 3;

        /*
        hardcoding tables 256->265 for kernel
        */
        for(int i = 256; i < 270; i++) {
            page = physical.allocatePage(1);
            physical.finishAllocation(page, 1);
            table = static_cast<PageTable*>(reinterpret_cast<void*>(page));
            memset(table, 0, PageSize);
            directory->pageTableAddresses[i] = page | 3;
        }

        /*page = physical.allocatePage(1);
        physical.finishAllocation(page, 1);
        table = static_cast<PageTable*>(reinterpret_cast<void*>(page));
        memset(table, 0, PageSize);
        directory->pageTableAddresses[1023] = page | 3;
        for (int i = 0; i < 1023; i++) {
            table->pageAddresses[i] = 
        }
        table->pageAddresses[1023] = reinterpret_cast<uintptr_t>(static_cast<void*>(directory)) | 3; 
        */
        directory->pageTableAddresses[1023] = reinterpret_cast<uintptr_t>(static_cast<void*>(directory)) | 3;
    }

    void updateCR3Address(PageDirectory* directory) {
        //auto directoryAddress = reinterpret_cast<uintptr_t>(static_cast<void*>(directory));
        auto directoryAddress = directory->pageTableAddresses[1023] & ~0x3ff;
        
        asm("movl %%eax, %%CR3 \n"
            : //no output
            : "a" (directoryAddress)
        );
    }

    uintptr_t VirtualMemoryManager::allocatePageTable(uintptr_t virtualAddress, int index) {
        auto physicalPage = physicalManager.allocatePage(1);

        auto table = static_cast<PageTable*>(reinterpret_cast<void*>(physicalPage));
        memset(table, 0, PageSize);
        directory->pageTableAddresses[index] = physicalPage | 3;

        updateCR3Address(directory);

        physicalManager.finishAllocation(virtualAddress, 1);

        return physicalPage;
    }

    uintptr_t VirtualMemoryManager::allocatePages(uint32_t count) {
        /*auto physicalAddress = physicalManager.allocatePage(1);
        map(virtualAddress, physicalAddress);
        physicalManager.finishAllocation(virtualAddress, 1);*/

        /*
        mark nextAddress -> nextAddress + PageSize * count in the directory
        */
        //TODO: use avl tree to assign virtual address
        auto startingAddress = nextAddress;
        auto virtualAddress = nextAddress;
        nextAddress += PageSize * count;

        for(uint32_t i = 0; i < count; i++) {
            auto page = physicalManager.allocatePage(1);
            
            //table = static_cast<PageTable*>(reinterpret_cast<void*>(page));
            //memset(table, 0, PageSize);

            auto directoryIndex = virtualAddress >> 22;

            if (directory->pageTableAddresses[directoryIndex] == 0) {
                allocatePageTable(nextAddress, directoryIndex);
                nextAddress += PageSize;
            }

            //auto pageTableAddress = directory->pageTableAddresses[directoryIndex] & 0xFFFFF000;
            auto pageTableAddress = 0xFFC00000 + PageSize * directoryIndex;
            auto pageTable = static_cast<PageTable*>(reinterpret_cast<void*>(pageTableAddress));

            auto tableIndex = (virtualAddress >> 12) & 0x3FF;
            //should present be 0 here?
            pageTable->pageAddresses[tableIndex] = page | 3; //page | 3;

            virtualAddress += PageSize;
        }

        updateCR3Address(directory);

        return startingAddress;
    }

    void VirtualMemoryManager::map(uintptr_t virtualAddress, uintptr_t physicalAddress, uint32_t pageCount) {
        

        for (auto i = 0u; i < pageCount; i++) {
            auto directoryIndex = virtualAddress >> 22;
            auto pageTableAddress = directory->pageTableAddresses[directoryIndex] & 0xFFFFF000;
            auto pageTable = static_cast<PageTable*>(reinterpret_cast<void*>(pageTableAddress));
            
            auto tableIndex = (virtualAddress >> 12) & 0x3FF;
            pageTable->pageAddresses[tableIndex] = physicalAddress | 3;

            //printf("[VMM] Mapped %d at %d in dir: %d, table: %d\n", 
            //    physicalAddress, virtualAddress, directoryIndex, tableIndex);

            virtualAddress += PageSize;
            physicalAddress += PageSize;
        }

        if (virtualAddress > nextAddress) {
            nextAddress = virtualAddress;
        }
    }

    void VirtualMemoryManager::activate() {
        //auto directoryAddress = reinterpret_cast<uintptr_t>(static_cast<void*>(&directory));
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
    }

}