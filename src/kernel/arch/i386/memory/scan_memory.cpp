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
#include "scan_memory.h"
#include <stdio.h>

extern uint32_t __kernel_memory_start;
extern uint32_t __kernel_memory_end;

namespace Memory {
    void scan(uintptr_t memoryMapAddress, uint32_t memoryMapLength) {
        auto memoryMap = static_cast<MemoryMapRecord*>(reinterpret_cast<void*>(memoryMapAddress));
        auto count = memoryMapLength / sizeof(MemoryMapRecord);

        char types[5][20] = {"Usable RAM", "Reserved", "ACPI (reclaimable)", "ACPI (nvs)", "Bad"};

        auto kernelEndAddress = reinterpret_cast<uint32_t>(&__kernel_memory_end);
        auto firstPage = (kernelEndAddress & 0xFFFFF000) + 0x1000;
        kprintf("First page will be located at: %x\n", firstPage);

        for(auto i = 0u; i < count; i++) {
            auto record = *memoryMap;
            if (record.lowerBaseAddress > firstPage && record.type == 1) {
                kprintf("\nMemoryMapRecord\n----------------\n");
                kprintf("Size: %d, Type: %d %s, ", record.size, record.type, types[record.type]);
                kprintf("Addr_L: %x, Addr_H: %x, ", record.lowerBaseAddress, record.higherBaseAddress);
                kprintf("Len_L: %x, Len_H: %x\n", record.lowerLength, record.higherLength);
            }

            memoryMap++;
        }
    }
}