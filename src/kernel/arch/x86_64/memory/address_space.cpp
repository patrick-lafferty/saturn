/*
Copyright (c) 2018, Patrick Lafferty
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

#include "address_space.h"
#include <cpu/metablocks.h>
#include "virtual_memory_manager.h"
#include "physical_memory_manager.h"

namespace Memory {

    AddressReservation::AddressReservation(VirtualAddress start, uint64_t size)
        : start{start}, size {size} {
        
        nextPage = start;
        remainingPages = size / 0x1000;
    }

    std::optional<VirtualAddress> AddressReservation::allocatePages(uint64_t count) {
        
        if (remainingPages < count) {
            return {};
        }

        remainingPages -= count;
        auto page = nextPage;
        nextPage.address += count * 0x1000;

        return page;
    }

    AddressReservation AddressReservation::split(uint64_t requiredSize) {
        AddressReservation sibling {{start.address + requiredSize}, this->size - requiredSize};
        this->size = requiredSize;
        this->remainingPages = this->size / 0x1000;

        return sibling;
    }

    uint64_t AddressReservation::getSize() const {
        return size;
    }

    void AddressReservation::clearFreeFlag() {
        isFree = false;
    }

    bool AddressReservation::isAvailable() const {
        return isFree;
    }

    AddressSpace::Allocator::Allocator() {
    }

    void AddressSpace::Allocator::preparePage(size_t itemSize) {
        auto& core = CPU::getCurrentCore();

        core.virtualMemory->allocatePagingTablesFor(currentPage, core.physicalMemory);
        currentItems = 0;
        availableItems = 0x1000 / itemSize;
    }

    AddressSpace::AddressSpace()
        : space {allocator} {
        
        AddressReservation initial {{0}, 2UL << 40};
        space.insert(initial);
    }

    std::optional<AddressReservation> AddressSpace::reserve(uint64_t size) {
        auto node = space.findByTraversal([=](auto& value) {
            return value.getSize() >= size && value.isAvailable();
        });

        if (node.has_value()) {
            auto result = node.value();
            space.remove(result); 

            if (result.getSize() > size) {
                auto remaining = result.split(size);
                space.insert(remaining);
            }
            
            result.clearFreeFlag();
            space.insert(result);

            return result;
        }
        else {
            return {};
        }
    }

    void AddressSpace::release(AddressReservation reservation) {
        //TODO: implement release
    }
}
