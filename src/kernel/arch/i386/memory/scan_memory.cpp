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