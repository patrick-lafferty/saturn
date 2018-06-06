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
#include "initialize.h"
#include <cpu/tss.h>
#include <cpu/acpi.h>
#include <cpu/apic.h>
#include <cpu/cpu.h>
#include <cpu/sse.h>
#include <cpu/pic.h>
#include <cpu/rtc.h>
#include <gdt/gdt.h>
#include <memory/physical_memory_manager.h>
#include <memory/virtual_memory_manager.h>
#include <task.h>
#include <scheduler.h>

using namespace Kernel;
using namespace Memory;

extern "C" void trampoline_start();
extern "C" uint8_t trampoline_size;

namespace CPU {

    void infiniteLoop() {
        while (true) {
            int x = 0;
            x++;
        }
    }

    void kernel_sleep(int milliseconds) {
        RTC::enable(5);

        while (milliseconds > 0) {
            asm("hlt");
            milliseconds--;
        }

        RTC::disable();
    }

    Trampoline createTrampoline(BlockAllocator<SmallStack>& stackAllocator,
            BlockAllocator<VirtualMemoryManager>& vmmAllocator,
            int cpuId) {

        auto stack = stackAllocator.allocate();
        auto vmm = vmmAllocator.allocate();
        currentVMM->cloneForUsermode(vmm, true);

        auto configMemoryBlock = reinterpret_cast<void*>(0x3000);
        auto savedPhysicalNextFreeAddress = *reinterpret_cast<uint32_t*>(configMemoryBlock);
        auto config = reinterpret_cast<uint8_t*>(configMemoryBlock);

        memcpy(configMemoryBlock, reinterpret_cast<void*>(trampoline_start), 
            trampoline_size);

        config += trampoline_size;
        auto startingGDTAddress = reinterpret_cast<uintptr_t>(config);
        memcpy(config, gdt, sizeof(GDT::Descriptor) * 6);
        config += sizeof(GDT::Descriptor) * 6;

        auto gdtPointer = reinterpret_cast<GDT::DescriptorPointer*>(config);
        gdtPointer->limit = sizeof(GDT::Descriptor) * 6 - 1;
        gdtPointer->base = startingGDTAddress;

        auto cpuData = ((int*)configMemoryBlock) + 0x3Fb;
        auto status = cpuData++;
        *status = 0;

        auto stackPointer = reinterpret_cast<TrampolineStack*>(
            reinterpret_cast<uintptr_t>(stack) + sizeof(SmallStack) - 8
        );
        *stackPointer = {
            reinterpret_cast<uintptr_t>(status),
            reinterpret_cast<uintptr_t>(infiniteLoop) 
        };

        auto data = reinterpret_cast<TrampolineData*>(cpuData);
        *data = {
            reinterpret_cast<uintptr_t>(gdtPointer),
            vmm->getDirectoryPhysicalAddress(),
            reinterpret_cast<uintptr_t>(stackPointer)
        };

        return {data, stackPointer, status, savedPhysicalNextFreeAddress};
    }
    
    void replaceTrampolineStack(Trampoline& trampoline, BlockAllocator<SmallStack>& stackAllocator,
            BlockAllocator<VirtualMemoryManager>& vmmAllocator,
            int cpuId) {

        auto stack = stackAllocator.allocate();
        auto vmm = vmmAllocator.allocate();
        currentVMM->cloneForUsermode(vmm, true);

        auto stackPointer = reinterpret_cast<TrampolineStack*>(
            reinterpret_cast<uintptr_t>(stack) + sizeof(SmallStack) - 8
        );

        *stackPointer = {
            reinterpret_cast<uintptr_t>(trampoline.status),
            reinterpret_cast<uintptr_t>(infiniteLoop) 
        };
        
        trampoline.data->pageDirectoryAddress = vmm->getDirectoryPhysicalAddress();
        trampoline.data->stackAddress = reinterpret_cast<uintptr_t>(stackPointer);        
    }

    void initializeApplicationProcessors() {

        BlockAllocator<SmallStack> stackAllocator {currentVMM};
        BlockAllocator<VirtualMemoryManager> vmmAllocator {currentVMM};

        auto trampoline = createTrampoline(stackAllocator, vmmAllocator, 1);

        APIC::sendInitIPI(1);
        kernel_sleep(10);
        APIC::sendStartupIPI(1, 0x3);
        kernel_sleep(1);

        //wait for up to 500ms
        int x = 50;

        while (*trampoline.status != 1 && x > 0) {
            kernel_sleep(10);
            x -= 1;
        }

        if (*trampoline.status != 1) {
            //this cpu failed to start
        }

        *trampoline.status = 1337;

        while (*trampoline.status <= 9000) {
            kernel_sleep(1);
        }

        /*
        TODO:

        free the physical page, use trampoline.savedPhysicalNextAddress
        */
    }

    TSS* createTSS(uint32_t kernelEndAddress) {

        auto pageFlags = static_cast<int>(PageTableFlags::AllowWrite);
        auto afterKernel = (kernelEndAddress & ~0xfff) + 0x1000;

        Memory::currentVMM->HACK_setNextAddress(0xCFFF'F000);
        auto tssAddress = Memory::currentVMM->allocatePages(1, pageFlags);
        Memory::currentVMM->HACK_setNextAddress(afterKernel);

        TSS_ADDRESS = tssAddress;
        GDT::addTSSEntry(tssAddress, PageSize);
        auto tss = setupTSS(tssAddress);

        return tss;
    }

    Kernel::Scheduler* initialize(uint32_t kernelEndAddress) {

        initializeSSE();
        PIC::disable();

        asm volatile("sti");
        auto tss = createTSS(kernelEndAddress);

        if (!CPU::parseACPITables()) {

            kprintf("[Kernel] Parsing ACPI Tables failed, halting\n");
            asm volatile("hlt");
        }

        initializeApplicationProcessors();

        TaskLauncher launcher {tss};
        auto scheduler = BlockAllocator<Scheduler>(Memory::currentVMM).allocate();
        setupCPUBlocks(1, {scheduler});

        return scheduler;
    }

}
