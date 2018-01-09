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
#include "idt.h"
#include <string.h>
#include <stdio.h>
#include <memory/physical_memory_manager.h>
#include <memory/virtual_memory_manager.h>
#include <cpu/apic.h>
#include <scheduler.h>
#include <system_calls.h>
#include <services/ps2/ps2.h>
#include <services.h>

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
        idt[207] = encodeEntry(reinterpret_cast<uint32_t>(&isr207), 0x08);

        //System calls
        idt[255] = encodeEntry(reinterpret_cast<uint32_t>(&isr255), 0x08, true);

        loadIDT();
    }
    
    Entry encodeEntry(uint32_t base, uint16_t kernelSegment, bool isUserspaceCallable) {
        Entry entry;

        entry.baseLow = base & 0xFFFF;
        entry.kernelSegment = kernelSegment;

        /*
        Flags bits:
        Bits 0 to 3: GateType 
            (1111 = 386 32-bit trap gate, 1110 = 386 32-bit interrupt gate,
                0111 = 286 16-bit trap gate, 0110 = 286 16-bit interrupt gate,
                0101 = 386 32-bit task gate)
        Bit 4: Storage Segment (0 for interrupt/trap gates)
        Bits 5 to 6: Descriptor Priviledge Level (minimum privilege level
            the calling descriptor must have)
        Bit 7: Present (0 for unused interupts, 1 for used)

        For the majority of interrupts, we want
        0b10001110 = 0x8E
        */
        entry.flags = 0x8E;

        if (isUserspaceCallable) {
            /*
            Set the DPL (see above) to 0b11 for ring3
            */
            entry.flags |= 0x60;
        }

        entry.baseHigh = (base & 0xFFFF0000) >> 16;

        return entry;
    }
}

enum class PageFaultError {
    ProtectionViolation = 1 << 0,
    WriteAccess = 1 << 1,
    UserMode = 1 << 2
};

void panic(CPU::InterruptStackFrame* frame) {
    asm volatile("cli");

    kprintf("[IDT] General Protection Fault\n");
            kprintf("GS: %x, FS: %x, ES: %x, DS: %x\n", 
                frame->gs, frame->fs, frame->es, frame->ds);
            kprintf("EDI: %x, ESI: %x, EBP: %x, ESP: %x\n", 
                frame->edi, frame->esi, frame->ebp, frame->esp);
            kprintf("EBX: %x, EDX: %x, ECX: %x, EAX: %x\n", 
                frame->ebx, frame->edx, frame->ecx, frame->eax);
            kprintf("Error code: %x, EIP: %x, CS: %x, EFLAGS: %x\n", 
                frame->errorCode, frame->eip, frame->cs, frame->eflags);
            kprintf("RESP: %x, SS: %x\n", 
                frame->resp, frame->ss);

    Kernel::currentScheduler->getCurrentTask()->virtualMemoryManager->activate();

    asm volatile("hlt");
}

bool handlePageFault(uintptr_t virtualAddress, uint32_t errorCode) {

    if (errorCode & static_cast<uint32_t>(PageFaultError::ProtectionViolation)) {
        kprintf("[IDT] Page fault: protection violation [%x]\n", virtualAddress);
        
        if (errorCode & static_cast<uint32_t>(PageFaultError::UserMode)) {
            kprintf("[IDT] Attempted %s in usermode\n",
                errorCode & static_cast<uint32_t>(PageFaultError::WriteAccess)
                    ? "write" : "read");
        }

        asm volatile("hlt");
    }
    else {

        auto pageStatus = Memory::currentVMM->getPageStatus(virtualAddress);
        
        if (pageStatus == Memory::PageStatus::Allocated) {
            //we need to map a physical page
            auto physicalPage = Memory::currentPMM->allocatePage(1);
            Memory::currentVMM->map(virtualAddress, physicalPage);
            Memory::currentPMM->finishAllocation(virtualAddress, 1);

            return true;
        }
        else if (pageStatus == Memory::PageStatus::Mapped) {
            //this shouldn't happen?
            kprintf("[IDT] Page Fault: mapped address?\n");
            asm volatile("hlt");
        }
        else {
            kprintf("[IDT] Illegal Memory Access: %x\n", virtualAddress);     
            asm volatile("hlt");
        }
    }

    return false;
}

void handleSystemCall(CPU::InterruptStackFrame* frame) {

    switch(frame->eax) {
        case static_cast<uint32_t>(SystemCall::Exit): {
            Kernel::currentScheduler->exitTask();
            break;
        }
        case static_cast<uint32_t>(SystemCall::Sleep): {
            Kernel::currentScheduler->blockTask(Kernel::BlockReason::Sleep, frame->ebx);
            break;
        }
        case static_cast<uint32_t>(SystemCall::Send): {
            Kernel::currentScheduler->sendMessage(static_cast<IPC::RecipientType>(frame->ebx), reinterpret_cast<IPC::Message*>(frame->ecx));
            break;
        }
        case static_cast<uint32_t>(SystemCall::Receive): {
            Kernel::currentScheduler->receiveMessage(reinterpret_cast<IPC::Message*>(frame->ebx));
            break;
        }
        default: {
            kprintf("[IDT] Unhandled system call: %d (ebx: %x)\n", frame->eax, frame->ebx);
            break;
        }
    }
}

const char* exceptions[] = {
    "Divide Error",
    "Debug Exception",
    "Non-Maskable External Interrupt",
    "Breakpoint",
    "Overflow",
    "Bound Range Exceeded",
    "Invalid/Undefined Opcode",
    "Device not available [math coprocessor]",
    "Double Fault",
    "Coprocessor Segment Overrun",
    "Invalid TSS",
    "Segment not present"
    "Stack-Segment Fault",
    "General Protection Fault",
    "Page Fault",
    "Reserved [15]",
    "x87 FPU Error",
    "Alignment Check",
    "Machine Check",
    "SIMD Floating-Point Exception",
    "Virtualization Exception"
};

extern "C" void taskSwitch() {
    APIC::signalEndOfInterrupt();
    Kernel::currentScheduler->notifyTimesliceExpired(); 
}

void interruptHandler(CPU::InterruptStackFrame* frame) {
    
    switch(frame->interruptNumber) {
        case 0: {
            kprintf("[IDT] Divide Error\n");
            break;
        }
        case 6: {
            kprintf("[IDT] %s\n", exceptions[frame->interruptNumber]);
            panic(frame);
        }
        //...TODO...
        case 13: {
            kprintf("[IDT] General Protection Fault\n");
            kprintf("GS: %x, FS: %x, ES: %x, DS: %x\n", 
                frame->gs, frame->fs, frame->es, frame->ds);
            kprintf("EDI: %x, ESI: %x, EBP: %x, ESP: %x\n", 
                frame->edi, frame->esi, frame->ebp, frame->esp);
            kprintf("EBX: %x, EDX: %x, ECX: %x, EAX: %x\n", 
                frame->ebx, frame->edx, frame->ecx, frame->eax);
            kprintf("Error code: %x, EIP: %x, CS: %x, EFLAGS: %x\n", 
                frame->errorCode, frame->eip, frame->cs, frame->eflags);
            kprintf("RESP: %x, SS: %x\n", 
                frame->resp, frame->ss);
            asm volatile("hlt");
            break;
        }
        case 14: {
            //page fault
            uintptr_t virtualAddress;
            asm("movl %%CR2, %%eax" : "=a"(virtualAddress));

            if (!handlePageFault(virtualAddress, frame->errorCode)) {
                panic(frame);
            } 
            break;
        }
        case 48:
        case 49:
        case 50:
        case 51:
        case 52:
        case 53:
        case 54:
        case 55:
        case 56:
        case 57:
        case 58:
        case 59: {
            //io apic irq

            if (frame->interruptNumber == 49) {
                uint8_t full {0};
                uint16_t statusRegister {0x64};
                uint16_t dataPort {0x60};

                //TODO: if this is slow, put this in the ps2 service. can't right now since IOPB isn't supported
                do {
                     asm("inb %1, %0"
                        : "=a" (full)
                        : "Nd" (statusRegister));

                    if (full & 0x1) {
                        uint8_t c;
                        asm("inb %1, %0"
                            : "=a" (c)
                            : "Nd" (dataPort));

                        PS2::KeyboardInput input {};
                        input.data = c;
                        input.serviceType = Kernel::ServiceType::PS2;
                        Kernel::currentScheduler->sendMessage(IPC::RecipientType::ServiceName, &input);
                    }

                } while (full & 0x1);
            }
            else if (frame->interruptNumber == 51) {
                if (APIC::calibrateAPICTimer()) {
                    Kernel::currentScheduler->setupTimeslice();
                }
            }
            else if (frame->interruptNumber == 52) {
                APIC::signalEndOfInterrupt();
                Kernel::currentScheduler->notifyTimesliceExpired();
                return;
            }
            else if (!Kernel::ServiceRegistryInstance->handleDriverIrq(frame->interruptNumber)) {
                kprintf("[IDT] Unhandled APIC IRQ %d\n", frame->interruptNumber);
            }
            APIC::signalEndOfInterrupt();
            break;
        }
        case 255: {
            handleSystemCall(frame);

            break;
        }
        default: {
            kprintf("[IDT] Unhandled interrupt %d, ", frame->interruptNumber);

            if (frame->interruptNumber < 21) {
                kprintf(exceptions[frame->interruptNumber]);
            }

            kprintf("\n");

            asm volatile("hlt");
            break;
        }
    }
}