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

    AddressSpace::Allocator::Allocator() {
        preparePage();
    }

    void AddressSpace::Allocator::preparePage() {
        auto& core = CPU::getCurrentCore();
        auto frame = core.physicalMemory->allocatePage();
        auto flags = 3;
        core.virtualMemory->map(currentPage, frame, flags);
        core.physicalMemory->finishAllocation(currentPage);
        currentItems = 0;
    }

    AddressSpace::AddressSpace()
        : space {allocator} {
        
        AddressReservation initial {0, 2UL << 40};
        space.insert(initial);
    }

    std::optional<AddressReservation> AddressSpace::reserve(uint64_t size) {
        auto node = space.findByTraversal([=](auto& value) {
            return value.size >= size && value.isFree;
        });

        if (node.has_value()) {
            auto result = node.value();
            space.remove(result); 

            if (result.size > size) {
                AddressReservation remaining {result.startAddress + size, result.size - size};
                space.insert(remaining);
                result.size = size;
            }
            
            result.isFree = false;
            space.insert(result);

            return result;
        }
        else {
            return {};
        }
    }
}