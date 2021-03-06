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
#include "virtual_memory_manager.h"
#include "physical_memory_manager.h"
#include "address_space.h"
#include <cpu/metablocks.h>

namespace Memory {

	struct PagingIndex {
		int level4;
		int directoryPointer;
		int directory;
		int table;
	};

	PagingIndex calculateIndex(uintptr_t virtualAddress) {
		PagingIndex index;
		
		index.level4 = (virtualAddress >> 39) & 511;
		index.directoryPointer = (virtualAddress >> 30) & 511;
		index.directory = (virtualAddress >> 21) & 511;
		index.table = (virtualAddress >> 12) & 511;

		return index;
	}

	constexpr uintptr_t SignExtension = 0xFFFFul << 48;
	constexpr uintptr_t PageTableStartAddress = 
		SignExtension
		+ (510ul << 39);
	constexpr uintptr_t PageDirectoryStartAddress = 
		PageTableStartAddress
		+ (510ul << 30);
	constexpr uintptr_t PageDirectoryPointerStartAddress = 
		PageDirectoryStartAddress
		+ (510ul << 21);
	constexpr uintptr_t Level4StartAddress = 
		PageDirectoryPointerStartAddress	
		+ (510ul << 12);

	PhysicalAddress reserveIndex(VirtualAddress linear, PagingIndex index, PhysicalMemoryManager* physicalMemory) {
		auto& map = *reinterpret_cast<volatile PageMapLevel4*>(Level4StartAddress);
		auto nextAddress = PageDirectoryPointerStartAddress + (((linear.address) >> 27) & 0x00001ff000);

		if (map.directoryPointers[index.level4] == 0) {
			auto physicalPage = physicalMemory->allocatePage();
			map.directoryPointers[index.level4] = physicalPage.address | 3;
			physicalMemory->finishAllocation({nextAddress});
		}

		auto& directoryPointer = *reinterpret_cast<volatile PageDirectoryPointer*>(nextAddress);
		nextAddress = PageDirectoryStartAddress + (((linear.address) >> 18) & 0x003ffff000);
		
		if (directoryPointer.directoryTables[index.directoryPointer] == 0) {
			auto physicalPage = physicalMemory->allocatePage();
			directoryPointer.directoryTables[index.directoryPointer] = physicalPage.address | 3;
			physicalMemory->finishAllocation({nextAddress}); 
		}

		auto& directory = *reinterpret_cast<volatile PageDirectoryTable*>(nextAddress);
		nextAddress = PageTableStartAddress + (((linear.address) >> 9) & 0x7ffffff000);

		if (directory.pageTables[index.directory] == 0) {
			auto physicalPage = physicalMemory->allocatePage();
			directory.pageTables[index.directory] = physicalPage.address | 3;
			physicalMemory->finishAllocation({nextAddress});
		}

		auto& table = *reinterpret_cast<volatile PageTable*>(nextAddress);

		if (table.pages[index.table] == 0) {
			auto physicalPage = physicalMemory->allocatePage();
			table.pages[index.table] = physicalPage.address | 3;
			physicalMemory->finishAllocation({linear.address & ~0xFFF});

            return physicalPage;
		}
        else {
            return {table.pages[index.table] & ~3};
        }
	}

	void VirtualMemoryManager::map(VirtualAddress linear, PhysicalAddress physical, uint32_t flags) {

		auto index = (linear.address >> 12) & 511;
		auto tableAddress = PageTableStartAddress + ((linear.address >> 9) & 0x7FFFFFF000);
		auto table = reinterpret_cast<volatile PageTable*>(tableAddress);
		table->pages[index] = physical.address | 3;
	}

	void VirtualMemoryManager::unmap(VirtualAddress linear) {
		auto index = (linear.address >> 12) & 511;
		uintptr_t tableAddress = PageTableStartAddress + (linear.address >> 9) & 0x7FFFFFF000;
		auto table = reinterpret_cast<volatile PageTable*>(tableAddress);
		table->pages[index] = 0;
	}

	PhysicalAddress VirtualMemoryManager::allocatePagingTablesFor(VirtualAddress linear, 
            PhysicalMemoryManager* physicalMemoryManager) {
		auto index = calculateIndex(linear.address);
		return reserveIndex(linear, index, physicalMemoryManager);	
	}

	std::optional<PhysicalAddress> VirtualMemoryManager::clone() {
		/*
		Clones need the first page table (first 2MB that's identity mapped),
		as well as the last PDP (511 = kernel)
		*/
		auto& core = CPU::getCurrentCore();
		//TODO: this should be a kernel address space
		auto maybeReservation = core.addressSpace->reserve(PageSize * 4);

		if (!maybeReservation.has_value()) {
			return {};
		}

		auto reservation = maybeReservation.value();
		auto maybeAllocation = reservation.allocatePages(4);

		if (!maybeAllocation.has_value()) {
			return {};
		}

		auto allocation = maybeAllocation.value();

        auto pml4PhysicalAddress = allocatePagingTablesFor(allocation, core.physicalMemory);
        allocatePagingTablesFor({allocation.address + PageSize * 1}, core.physicalMemory);
        allocatePagingTablesFor({allocation.address + PageSize * 2}, core.physicalMemory);
        allocatePagingTablesFor({allocation.address + PageSize * 3}, core.physicalMemory);

		auto pageMapLevel4 = reinterpret_cast<uintptr_t*>(allocation.address);
		auto pageDirectoryPointer = reinterpret_cast<uintptr_t*>(allocation.address + PageSize);
		auto pageDirectory = reinterpret_cast<uintptr_t*>(allocation.address + PageSize * 2);
		auto pageTable = reinterpret_cast<uintptr_t*>(allocation.address + PageSize * 3);
		auto& currentMap = *reinterpret_cast<volatile PageMapLevel4*>(Level4StartAddress);

		auto flags = 3;

		*(pageMapLevel4 + 0) = reinterpret_cast<uintptr_t>(pageDirectoryPointer) | flags;
		*(pageMapLevel4 + 510) = reinterpret_cast<uintptr_t>(pageMapLevel4) | flags;
		*(pageMapLevel4 + 511) = currentMap.directoryPointers[511];
		*pageDirectoryPointer = reinterpret_cast<uintptr_t>(pageDirectory) | flags;
		*pageDirectory = reinterpret_cast<uint64_t>(pageTable) | flags;

		/*
		Identity-map the first 2 megabytes
		*/
		for (int i = 0; i < 512; i++) {
			auto page = 0x1000 * i;
			page |= flags;
			*pageTable++ = page;
		}

		return pml4PhysicalAddress;
	}

	PageStatus VirtualMemoryManager::getPageStatus(VirtualAddress linear) {
		auto index = (linear.address >> 12) & 511;
		auto tableAddress = PageTableStartAddress + ((linear.address >> 9) & 0x7FFFFFF000);
		auto table = reinterpret_cast<PageTable*>(tableAddress);
		auto physicalAddress = table->pages[index];

		if (physicalAddress & 0xFF) {
			if (physicalAddress & ~0xFF) {
				return PageStatus::Mapped;
			}
			else {
				return PageStatus::Allocated;
			}
		}
		else if (linear.address > PageTableStartAddress) {
			//this is a page table/directory/directory pointer
			return PageStatus::UnallocatedPageTable;
		}
		else {
			return PageStatus::Invalid;
		}
	}
}
