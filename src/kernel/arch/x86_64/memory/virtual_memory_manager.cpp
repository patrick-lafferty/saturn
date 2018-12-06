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

namespace Memory {

    VirtualMemoryManager::VirtualMemoryManager() { /*PhysicalMemoryManager* physicalMemory) {
        level4Address = physicalMemory->allocatePage();
        auto pageDirectoryPointer = physicalMemory->allocatePage();
        auto pageDirectory = physicalMemory->allocatePage();
        auto pageTable = physicalMemory->allocatePage();
		*/
    }

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

	void reserveIndex(uintptr_t virtualAddress, PagingIndex index, PhysicalMemoryManager* physicalMemory) {
		auto& map = *reinterpret_cast<volatile PageMapLevel4*>(Level4StartAddress);
		auto nextAddress = PageDirectoryPointerStartAddress + (((virtualAddress) >> 27) & 0x00001ff000);

		if (map.directoryPointers[index.level4] == 0) {
			auto physicalPage = physicalMemory->allocatePage();
			map.directoryPointers[index.level4] = physicalPage | 3;
			physicalMemory->finishAllocation(nextAddress);//Level4StartAddress + 0x1000 * index.level4);
		}

		auto& directoryPointer = *reinterpret_cast<volatile PageDirectoryPointer*>(nextAddress);
		nextAddress = PageDirectoryStartAddress + (((virtualAddress) >> 18) & 0x003ffff000);
		
		if (directoryPointer.directoryTables[index.directoryPointer] == 0) {
			auto physicalPage = physicalMemory->allocatePage();
			directoryPointer.directoryTables[index.directoryPointer] = physicalPage | 3;
			physicalMemory->finishAllocation(nextAddress); //PageDirectoryPointerStartAddress + 0x1000 * index.directoryPointer);
		}

		auto& directory = *reinterpret_cast<volatile PageDirectoryTable*>(nextAddress);
		nextAddress = PageTableStartAddress + (((virtualAddress) >> 9) & 0x7ffffff000);

		if (directory.pageTables[index.directory] == 0) {
			auto physicalPage = physicalMemory->allocatePage();
			directory.pageTables[index.directory] = physicalPage | 3;
			physicalMemory->finishAllocation(nextAddress);//PageDirectoryStartAddress + 0x1000 * index.directory);
		}

		auto& table = *reinterpret_cast<volatile PageTable*>(nextAddress);

		if (table.pages[index.table] == 0) {
			auto physicalPage = physicalMemory->allocatePage();
			table.pages[index.table] = physicalPage | 3;
			physicalMemory->finishAllocation(virtualAddress & ~0xFFF);//PageTableStartAddress + 0x1000 * index.table);
		}
	}

	void VirtualMemoryManager::map(uintptr_t virtualAddress, uintptr_t physicalAddress, uint32_t flags) {

		auto index = (virtualAddress >> 12) & 511;
		auto tableAddress = PageTableStartAddress + ((virtualAddress >> 9) & 0x7FFFFFF000);
		auto table = reinterpret_cast<volatile PageTable*>(tableAddress);
		table->pages[index] = physicalAddress | 3;
	}

	void VirtualMemoryManager::unmap(uintptr_t virtualAddress, int count) {
		auto index = (virtualAddress >> 12) & 511;
		uintptr_t tableAddress = PageTableStartAddress + (virtualAddress >> 9) & 0x7FFFFFF000;
		auto table = reinterpret_cast<volatile PageTable*>(tableAddress);
		table->pages[index] = 0;
	}

	void VirtualMemoryManager::allocatePagingTablesFor(uintptr_t virtualAddress, PhysicalMemoryManager* physicalMemoryManager) {
		auto index = calculateIndex(virtualAddress);
		reserveIndex(virtualAddress, index, physicalMemoryManager);	
	}

	PageStatus VirtualMemoryManager::getPageStatus(uintptr_t virtualAddress) {
		auto index = (virtualAddress >> 12) & 511;
		auto tableAddress = PageTableStartAddress + ((virtualAddress >> 9) & 0x7FFFFFF000);
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
		else if (virtualAddress > PageTableStartAddress) {
			//this is a page table/directory/directory pointer
			return PageStatus::UnallocatedPageTable;
		}
		else {
			return PageStatus::Invalid;
		}
	}
}
