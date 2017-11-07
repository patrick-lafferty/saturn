#include "idt.h"
#include <string.h>
#include <stdio.h>
#include <memory/physical_memory_manager.h>
#include <memory/virtual_memory_manager.h>
#include <cpu/apic.h>
#include <scheduler.h>

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

        //remapped PIC, IRQs 0-15 = 32-47
        idt[32] = encodeEntry(reinterpret_cast<uint32_t>(&isr32), 0x08);
        idt[33] = encodeEntry(reinterpret_cast<uint32_t>(&isr33), 0x08);
        idt[34] = encodeEntry(reinterpret_cast<uint32_t>(&isr34), 0x08);
        idt[35] = encodeEntry(reinterpret_cast<uint32_t>(&isr35), 0x08);
        idt[36] = encodeEntry(reinterpret_cast<uint32_t>(&isr36), 0x08);
        idt[37] = encodeEntry(reinterpret_cast<uint32_t>(&isr37), 0x08);
        idt[38] = encodeEntry(reinterpret_cast<uint32_t>(&isr38), 0x08);
        idt[39] = encodeEntry(reinterpret_cast<uint32_t>(&isr39), 0x08);
        idt[40] = encodeEntry(reinterpret_cast<uint32_t>(&isr40), 0x08);
        idt[41] = encodeEntry(reinterpret_cast<uint32_t>(&isr41), 0x08);
        idt[42] = encodeEntry(reinterpret_cast<uint32_t>(&isr42), 0x08);
        idt[43] = encodeEntry(reinterpret_cast<uint32_t>(&isr43), 0x08);
        idt[44] = encodeEntry(reinterpret_cast<uint32_t>(&isr44), 0x08);
        idt[45] = encodeEntry(reinterpret_cast<uint32_t>(&isr45), 0x08);
        idt[46] = encodeEntry(reinterpret_cast<uint32_t>(&isr46), 0x08);
        idt[47] = encodeEntry(reinterpret_cast<uint32_t>(&isr47), 0x08);

        //APIC
        idt[48] = encodeEntry(reinterpret_cast<uint32_t>(&isr48), 0x08);
        idt[49] = encodeEntry(reinterpret_cast<uint32_t>(&isr49), 0x08);
        idt[50] = encodeEntry(reinterpret_cast<uint32_t>(&isr50), 0x08);
        idt[51] = encodeEntry(reinterpret_cast<uint32_t>(&isr51), 0x08);
        idt[52] = encodeEntry(reinterpret_cast<uint32_t>(&isr52), 0x08);
        idt[53] = encodeEntry(reinterpret_cast<uint32_t>(&isr53), 0x08);
        idt[54] = encodeEntry(reinterpret_cast<uint32_t>(&isr54), 0x08);
        idt[55] = encodeEntry(reinterpret_cast<uint32_t>(&isr55), 0x08);
        idt[56] = encodeEntry(reinterpret_cast<uint32_t>(&isr56), 0x08);
        idt[57] = encodeEntry(reinterpret_cast<uint32_t>(&isr57), 0x08);
        idt[58] = encodeEntry(reinterpret_cast<uint32_t>(&isr58), 0x08);
        idt[59] = encodeEntry(reinterpret_cast<uint32_t>(&isr59), 0x08);
        idt[255] = encodeEntry(reinterpret_cast<uint32_t>(&isr255), 0x08);

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
        printf("[IDT] Illegal Memory Access: %x\n", virtualAddress);     
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
    else if (frame->interruptNumber >= 48 && frame->interruptNumber <= 59) {
        //io apic irq

        if (frame->interruptNumber == 49) {
            uint8_t full {1};
            uint16_t statusRegister {0x64};
            uint16_t dataPort {0x60};

            printf("[IDT] Keyboard ");

            while (full & 0x1) {
                uint8_t c;
                asm("inb %1, %0"
                    : "=a" (c)
                    : "Nd" (dataPort));

                printf("%d ", c);

                asm("inb %1, %0"
                    : "=a" (full)
                    : "Nd" (statusRegister));

            }

            printf("\n");
        }
        else if (frame->interruptNumber == 51) {
            APIC::calibrateAPICTimer();
        }
        else if (frame->interruptNumber == 52) {
            
            APIC::signalEndOfInterrupt();
            Kernel::currentScheduler->notifyTimesliceExpired();
            return;
        }
        else {
            printf("[IDT] Unhandled APIC IRQ %d\n", frame->interruptNumber);
        }
        APIC::signalEndOfInterrupt();

    }
    else {
        printf("[IDT] Unhandled interrupt %d\n", frame->interruptNumber);
    }
}