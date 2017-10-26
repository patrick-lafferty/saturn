#include "idt.h"
#include <string.h>
#include <stdio.h>
#include <memory/physical_memory_manager.h>
#include <memory/virtual_memory_manager.h>

IDT::Entry idt[256];
IDT::EntryPointer idtPointer;

namespace IDT {
    void setup() {
        idtPointer.limit = sizeof(idt) - 1;
        idtPointer.base = reinterpret_cast<uint32_t>(&idt);

        memset(&idt, 0, sizeof(idt));

        idt[0] = encodeEntry(reinterpret_cast<uint32_t>(&isr0), 0x08);
        idt[1] = encodeEntry(reinterpret_cast<uint32_t>(&isr1), 0x08);
        idt[2] = encodeEntry(reinterpret_cast<uint32_t>(&isr2), 0x08);
        idt[3] = encodeEntry(reinterpret_cast<uint32_t>(&isr3), 0x08);
        idt[4] = encodeEntry(reinterpret_cast<uint32_t>(&isr4), 0x08);
        idt[5] = encodeEntry(reinterpret_cast<uint32_t>(&isr5), 0x08);
        idt[6] = encodeEntry(reinterpret_cast<uint32_t>(&isr6), 0x08);
        idt[7] = encodeEntry(reinterpret_cast<uint32_t>(&isr7), 0x08);
        idt[8] = encodeEntry(reinterpret_cast<uint32_t>(&isr8), 0x08);
        idt[9] = encodeEntry(reinterpret_cast<uint32_t>(&isr9), 0x08);
        idt[10] = encodeEntry(reinterpret_cast<uint32_t>(&isr10), 0x08);
        idt[11] = encodeEntry(reinterpret_cast<uint32_t>(&isr11), 0x08);
        idt[12] = encodeEntry(reinterpret_cast<uint32_t>(&isr12), 0x08);
        idt[13] = encodeEntry(reinterpret_cast<uint32_t>(&isr13), 0x08);
        idt[14] = encodeEntry(reinterpret_cast<uint32_t>(&isr14), 0x08);
        idt[15] = encodeEntry(reinterpret_cast<uint32_t>(&isr15), 0x08);
        idt[16] = encodeEntry(reinterpret_cast<uint32_t>(&isr16), 0x08);
        idt[17] = encodeEntry(reinterpret_cast<uint32_t>(&isr17), 0x08);
        idt[18] = encodeEntry(reinterpret_cast<uint32_t>(&isr18), 0x08);
        idt[19] = encodeEntry(reinterpret_cast<uint32_t>(&isr19), 0x08);
        idt[20] = encodeEntry(reinterpret_cast<uint32_t>(&isr20), 0x08);

        loadIDT();
    }
    
    Entry encodeEntry(uint32_t base, uint16_t kernelSegment) {
        Entry entry;

        entry.baseLow = base & 0xFFFF;
        entry.kernelSegment = kernelSegment;
        entry.flags = 0x8E;
        entry.baseHigh = (base & 0xFFFF0000) >> 16;

        return entry;
    }
}

void handlePageFault(uintptr_t virtualAddress) {
    auto pageStatus = Memory::currentVMM->getPageStatus(virtualAddress);
    
    if (pageStatus == Memory::PageStatus::Allocated) {
        //we need to map a physical page
        printf("[IDT] Page Fault: allocated page not mapped\n");
        auto physicalPage = Memory::currentPMM->allocatePage(1);
        Memory::currentVMM->map(virtualAddress, physicalPage);
        Memory::currentPMM->finishAllocation(virtualAddress, 1);
    }
    else if (pageStatus == Memory::PageStatus::Mapped) {
        //this shouldn't happen?
        printf("[IDT] Page Fault: mapped address?\n");
        while(true){}  
    }
    else {
        printf("[IDT] Illegal Memory Access\n");     
        while(true){}   
    }
}

void interruptHandler(CPU::InterruptStackFrame* frame) {

    if (frame->interruptNumber == 14) {
        //page fault
        uintptr_t virtualAddress;
        asm("movl %%CR2, %%eax" : "=a"(virtualAddress));

        handlePageFault(virtualAddress);        
    }    
}