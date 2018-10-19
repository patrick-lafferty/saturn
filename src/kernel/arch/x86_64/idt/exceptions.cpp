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
#include "exceptions.h"
#include "descriptor.h"
#include <cpu/metablocks.h>
#include <memory/physical_memory_manager.h>
#include <memory/virtual_memory_manager.h>
#include <log.h>

namespace IDT {

    void loadExceptions() {
        idt[0] = encodeEntry(reinterpret_cast<uintptr_t>(&exception0), 0x08);
        idt[1] = encodeEntry(reinterpret_cast<uintptr_t>(&exception1), 0x08);
        idt[2] = encodeEntry(reinterpret_cast<uintptr_t>(&exception2), 0x08);
        idt[3] = encodeEntry(reinterpret_cast<uintptr_t>(&exception3), 0x08);
        idt[4] = encodeEntry(reinterpret_cast<uintptr_t>(&exception4), 0x08);
        idt[5] = encodeEntry(reinterpret_cast<uintptr_t>(&exception5), 0x08);
        idt[6] = encodeEntry(reinterpret_cast<uintptr_t>(&exception6), 0x08);
        idt[7] = encodeEntry(reinterpret_cast<uintptr_t>(&exception7), 0x08);
        idt[8] = encodeEntry(reinterpret_cast<uintptr_t>(&exception8), 0x08);
        idt[9] = encodeEntry(reinterpret_cast<uintptr_t>(&exception9), 0x08);
        idt[10] = encodeEntry(reinterpret_cast<uintptr_t>(&exception10), 0x08);
        idt[11] = encodeEntry(reinterpret_cast<uintptr_t>(&exception11), 0x08);
        idt[12] = encodeEntry(reinterpret_cast<uintptr_t>(&exception12), 0x08);
        idt[13] = encodeEntry(reinterpret_cast<uintptr_t>(&exception13), 0x08);
        idt[14] = encodeEntry(reinterpret_cast<uintptr_t>(&exception14), 0x08);
        idt[15] = encodeEntry(reinterpret_cast<uintptr_t>(&exception15), 0x08);
        idt[16] = encodeEntry(reinterpret_cast<uintptr_t>(&exception16), 0x08);
        idt[17] = encodeEntry(reinterpret_cast<uintptr_t>(&exception17), 0x08);
        idt[18] = encodeEntry(reinterpret_cast<uintptr_t>(&exception18), 0x08);
        idt[19] = encodeEntry(reinterpret_cast<uintptr_t>(&exception19), 0x08);
        idt[20] = encodeEntry(reinterpret_cast<uintptr_t>(&exception20), 0x08);
    }
}

enum class Exception {
    DivideError,
    DebugException,
    NonMaskableExternalInterrupt,
    Breakpoint,
    Overflow,
    BoundRangeExceeded,
    InvalidOpcode,
    DeviceNotAvailable,
    DoubleFault,
    CoprocessorSegmentOverrun,
    InvalidTSS,
    SegmentNotPresent,
    StackSegmentFault,
    GeneralProtectionFault,
    PageFault,
    Reserved,
    FPUError,
    AlignmentCheck,
    MachineCheck,
    SIMDException,
    VirtualizationException
};

[[noreturn]]
void panic() {

    log("Saturn has encountered an unrecoverable error");

    while (true) {
        asm("cli");
        asm("hlt");
    }
}

bool handlePageFault(uintptr_t virtualAddress, uint32_t errorCode) {
    enum class PageFaultError {
        ProtectionViolation = 1 << 0,
        WriteAccess = 1 << 1,
        UserMode = 1 << 2
    };

    static int faults = 0;

    if (errorCode & static_cast<uint32_t>(PageFaultError::ProtectionViolation)) {
        log("[IDT] Page fault: protection violation [%x]", virtualAddress);
        
        if (errorCode & static_cast<uint32_t>(PageFaultError::UserMode)) {
            log("[IDT] Attempted %s in usermode",
                errorCode & static_cast<uint32_t>(PageFaultError::WriteAccess)
                    ? "write" : "read");
        }

        panic();
    }
    else {

        auto& core = CPU::getCurrentCore();

        auto pageStatus = core.virtualMemory->getPageStatus(virtualAddress);
        
        if (pageStatus == Memory::PageStatus::Allocated) {
            //we need to map a physical page
            auto physicalPage = core.physicalMemory->allocatePage();
            core.virtualMemory->map(virtualAddress, physicalPage);
            core.physicalMemory->finishAllocation(virtualAddress);

            faults++;

            return true;
        }
        else if (pageStatus == Memory::PageStatus::Mapped) {
            //this shouldn't happen?
            log("[IDT] Page Fault: mapped address?");
            panic();
        }
        else {
            log("[IDT] Illegal Memory Access: %x", virtualAddress);     
            panic();
        }
    }

    return false;
}

void exceptionHandler(ExceptionFrame* frame) {

    switch (static_cast<Exception>(frame->index)) {
        case Exception::DoubleFault: {
            log("Double fault");
            panic();
            break;
        }
        case Exception::InvalidTSS: {
            log("Invalid TSS");
            panic();
            break;
        }
        case Exception::GeneralProtectionFault: {
            log("General protection fault");
            panic();
            break;
        }
        case Exception::PageFault: {
            uintptr_t virtualAddress;
            asm("movq %%CR2, %%rax" : "=a" (virtualAddress));

            if (!handlePageFault(virtualAddress, frame->error)) {
                panic();
            }

            break;
        }
        default: {
            log("Unhandled exception %d", frame->index);
        }
    }
}
