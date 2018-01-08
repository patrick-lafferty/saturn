#include "virtual_memory_manager.h"
#include "physical_memory_manager.h"
#include <string.h>
#include <stdio.h>
#include <heap.h>
#include <scheduler.h>

#ifndef TARGET_PREKERNEL
void activateVMM(Kernel::Task* task) {
    task->virtualMemoryManager->activate();
    LibC_Implementation::KernelHeap = task->heap;
}
#endif

extern "C" void setCR3(uint32_t);

#if TARGET_PREKERNEL
namespace MemoryPrekernel {
#else
namespace Memory {
#endif

    VirtualMemoryManager* currentVMM;

    uint32_t extractDirectoryIndex(uintptr_t virtualAddress) {
        return virtualAddress >> 22;
    }

    uint32_t extractTableIndex(uintptr_t virtualAddress) {
        return (virtualAddress >> 12) & 0x3FF;
    }

    uintptr_t calculatePageTableAddress(uintptr_t virtualAddress) {
        auto directoryIndex = extractDirectoryIndex(virtualAddress);
        return 0xFFC00000 + PageSize * directoryIndex;
    }

    void VirtualMemoryManager::setPhysicalManager(PhysicalMemoryManager* physical) {
        physicalManager = physical;
    }

    void VirtualMemoryManager::initialize(PhysicalMemoryManager* physical) {
        physicalManager = physical;
        auto page = physical->allocatePage(1);
        physical->finishAllocation(page, 1);
        directory = static_cast<PageDirectory*>(reinterpret_cast<void*>(page));
        directoryPhysicalAddress = page;
        directory->pageTableAddresses[1023] = reinterpret_cast<uintptr_t>(static_cast<void*>(directory)) | 3; //| 3; //| 7;//3;
    }

    void updateCR3Address(PageDirectory* directory) {
        auto directoryAddress = directory->pageTableAddresses[1023] & ~0x3ff;
        
        /*asm("movl %%eax, %%CR3 \n"
            : //no output
            : "a" (directoryAddress)
        );*/
        setCR3(directoryAddress);
    }

    uintptr_t VirtualMemoryManager::allocatePageTable(uintptr_t virtualAddress, int index) {
        auto physicalPage = physicalManager->allocatePage(1);

        auto flags = 3;

        if ((index < 0x33F) || (virtualAddress < KernelVirtualStartingAddress)) {
            //usermode addresses are 0 to KernelVirtualStartingAddress, and should be
            //modifiable by user processes
            flags |= static_cast<uint32_t>(PageTableFlags::AllowUserModeAccess);
        }

        directory->pageTableAddresses[index] = physicalPage | flags;

        if (pagingActive) {
            updateCR3Address(directory);
            physicalManager->finishAllocation(virtualAddress, 1);
        }
        else {
            physicalManager->finishAllocation(physicalPage, 1);
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
            }

            if (pagingActive) {
                auto pageTableAddress = 0xFFC00000 + PageSize * directoryIndex;
                auto pageTable = static_cast<PageTable*>(reinterpret_cast<void*>(pageTableAddress));

                auto tableIndex = extractTableIndex(virtualAddress);

                if (virtualAddress < KernelVirtualStartingAddress) {
                    //usermode addresses are 0 to KernelVirtualStartingAddress, and should be
                    //modifiable by user processes
                    flags |= static_cast<uint32_t>(PageTableFlags::AllowUserModeAccess);
                }

                pageTable->pageAddresses[tableIndex] = flags; 
            }

            virtualAddress += PageSize;
        }

        //TODO: add dirty flag to check if this is necessary
        if (pagingActive) {
            updateCR3Address(directory);
        }

        return startingAddress;
    }

    #if TARGET_PREKERNEL
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
    #endif

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

    void VirtualMemoryManager::unmap(uintptr_t virtualAddress, uint32_t count) {
        auto end = virtualAddress + count * PageSize;
        for(auto address = virtualAddress; address < end; address += PageSize) {
            auto pageTableAddress = calculatePageTableAddress(address);
            auto pageTable = static_cast<PageTable*>(reinterpret_cast<void*>(pageTableAddress));
            auto tableIndex = extractTableIndex(address);

            pageTable->pageAddresses[tableIndex] = 0;   
        }

        updateCR3Address(directory);
    }

    void VirtualMemoryManager::freePages(uintptr_t virtualAddress, uint32_t count) {
        auto end = virtualAddress + count * 0x1000;
        for(auto address = virtualAddress; address < end; address += 0x1000) {

            auto directoryIndex = extractDirectoryIndex(address);

            //if the pageTable for this address wasn't even allocated, don't bother doing anything
            if (directory->pageTableAddresses[directoryIndex] != 0) {
                auto pageTableAddress = calculatePageTableAddress(address);
                auto pageTable = static_cast<PageTable*>(reinterpret_cast<void*>(pageTableAddress));
                auto tableIndex = extractTableIndex(address);

                if (getPageStatus(address) == PageStatus::Mapped) {
                    auto physicalAddress = pageTable->pageAddresses[tableIndex]; 
                    physicalManager->freePage(address, physicalAddress);
                } 

                pageTable->pageAddresses[tableIndex] &= 0xFF;  
            }

            
        }

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
        auto directoryAddress = directoryPhysicalAddress;

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

    VirtualMemoryManager* VirtualMemoryManager::cloneForUsermode() {
        auto virtualMemoryManager = new VirtualMemoryManager;
        auto newDirectoryAddress = physicalManager->allocatePage(1);
        auto virtualAddress = allocatePages(1, 0x2);
        map(virtualAddress, newDirectoryAddress);
        physicalManager->finishAllocation(virtualAddress, 1);
        auto newDirectory = reinterpret_cast<PageDirectory*>(virtualAddress);
        auto kernelStart = extractDirectoryIndex(0xD000'0000);

        for (auto i = 0; i < 1024; i++) {
            if (i >= kernelStart) {
                newDirectory->pageTableAddresses[i] = directory->pageTableAddresses[i];
            }
            else {
                newDirectory->pageTableAddresses[i] = 0;
            }
        }

        newDirectory->pageTableAddresses[1023] = newDirectoryAddress | 3; 
        unmap(virtualAddress, 1);

        virtualMemoryManager->physicalManager = physicalManager;
        virtualMemoryManager->directoryPhysicalAddress = newDirectoryAddress;
        virtualMemoryManager->nextAddress = 0xa000'0000;
        virtualMemoryManager->pagingActive = true;

        return virtualMemoryManager;
    }

    void VirtualMemoryManager::cleanup() {

        for(auto start = 0xa000'0000; start < KernelVirtualStartingAddress; start += 0x1000) {
            auto directoryIndex = extractDirectoryIndex(start);

            //if the pageTable for this address wasn't even allocated, don't bother doing anything
            if (directory->pageTableAddresses[directoryIndex] != 0) {
                auto status = getPageStatus(start);

                if (status == PageStatus::Mapped) {
                    freePages(start, 1);
                }
            }
        }

        physicalManager->freePage(0xFFFFF000, directoryPhysicalAddress);
    }

    void VirtualMemoryManager::cleanupClonePageTables(PageDirectory& cloneDirectory, uint32_t kernelStackAddress) {

        auto kernelPageTable = extractDirectoryIndex(kernelStackAddress);

        for (auto i = 0u; i < 0x3FF; i++) {
            if (i == kernelPageTable) {
                continue;
            }

            auto physicalAddress = directory->pageTableAddresses[i];
            auto clonePhysicalAddress = cloneDirectory.pageTableAddresses[i];
            if ((clonePhysicalAddress & 0xFF) && (clonePhysicalAddress & ~0xFF) && physicalAddress != clonePhysicalAddress) {
                auto address = 0xFFC00000 + PageSize * i;
                /*printf("[VMM] Wants to cleanup %x pg %x, kernel: %x\n", 
                    address,
                    clonePhysicalAddress,
                    directory->pageTableAddresses[i]);*/
                bool needsMap = !(physicalAddress & ~0xFF);

                if (needsMap) {
                    map(address, clonePhysicalAddress);
                }

                physicalManager->freePage(address, clonePhysicalAddress);

                if (needsMap) {
                    unmap(address, 1);
                }
            }
        }
    }

    void VirtualMemoryManager::preallocateKernelPageTables() {
        auto start = extractDirectoryIndex(0xD000'0000);

        for (auto i = start; i < 1023; i++) {
            if (directory->pageTableAddresses[i] == 0) {
                allocatePageTable(0xFFC00000 + PageSize * i, i);
            }
        }
    }

    PageDirectory VirtualMemoryManager::getPageDirectory() {
        return *directory;
    }

    void VirtualMemoryManager::sharePages(uint32_t ownerStartAddress, VirtualMemoryManager* recipientVMM, uint32_t recipientStartAddress, uint32_t count) {
        map(nextAddress, recipientVMM->directoryPhysicalAddress);
        auto recipientDirectory = static_cast<PageDirectory*>(reinterpret_cast<void*>(nextAddress));
        auto directoryIndex = extractDirectoryIndex(recipientStartAddress); 

        auto recipientPageTableAddress = recipientDirectory->pageTableAddresses[directoryIndex];
        map(nextAddress + PageSize, recipientPageTableAddress);

        auto recipientPageTable = reinterpret_cast<PageTable*>(nextAddress + PageSize);

        auto tableIndex = extractTableIndex(recipientStartAddress);

        auto pagesToShare = (tableIndex + count > 1023) ? 1024 : (tableIndex + count);
        auto pageTable = static_cast<PageTable*>(reinterpret_cast<void*>(calculatePageTableAddress(ownerStartAddress)));

        int pagesShared = 0;
        for (auto i = tableIndex; i < pagesToShare; i++) {
            recipientPageTable->pageAddresses[i] = pageTable->pageAddresses[i];
            pagesShared++;
        }

        unmap(nextAddress, 2);

        if (tableIndex + count > 1023) {
            //its split between two page tables
            count -= pagesShared;
            sharePages(ownerStartAddress + pagesShared * PageSize, recipientVMM, recipientStartAddress + pagesShared * PageSize,
                count);
        }
    
    }
}