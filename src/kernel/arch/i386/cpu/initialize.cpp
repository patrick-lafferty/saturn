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
#include <cpu/msr.h>
#include <gdt/gdt.h>
#include <memory/physical_memory_manager.h>
#include <memory/virtual_memory_manager.h>
#include <task.h>
#include <scheduler.h>
#include <locks.h>
#include <idt/idt.h>

using namespace Kernel;
using namespace Memory;

extern "C" void trampoline_start();
extern "C" uint8_t trampoline_size;

namespace CPU {

    void kernel_sleep(int milliseconds) {
        RTC::enable(5);

        while (milliseconds > 0) {
            asm("hlt");
            milliseconds--;
        }

        RTC::disable();
    }

    TSS* createTSS(uint32_t afterAddress, uint32_t tssAddress, int tssIndex) {

        auto pageFlags = static_cast<int>(PageTableFlags::AllowWrite);

        Memory::InitialKernelVMM->HACK_setNextAddress(tssAddress);
        Memory::InitialKernelVMM->allocatePages(1, pageFlags);
        auto physicalPage = Memory::currentPMM->allocatePage(1);
        Memory::InitialKernelVMM->map(tssAddress, physicalPage, 0);
        Memory::currentPMM->finishAllocation(tssAddress, 1);
        Memory::InitialKernelVMM->HACK_setNextAddress(afterAddress);

        GDT::addTSSEntry(tssAddress, PageSize);
        auto tss = setupTSS(tssAddress);

        return tss;
    }

    uint32_t KernelEndAddr;

    void applicationProcessorStartup(int cpuId, int apicId, uintptr_t vmmAddress) {

        gdt_flush();
        loadIDT();
        asm volatile("sti");
        initializeSSE();

        APIC::initialize();

        writeModelSpecificRegister(ModelSpecificRegister::IA32_TSC_AUX, cpuId, 0);
    
        while (ActiveCPUs == nullptr || !ActiveCPUs[cpuId].ready) {
            asm volatile("hlt");
        }

        auto vmm = reinterpret_cast<Memory::VirtualMemoryManager*>(vmmAddress);
        vmm->activate();

        loadTSS(0x28 + cpuId * 8);

        APIC::setAPICTimer(APIC::LVT_TimerMode::Periodic, 10);
        APIC::setupISAIRQs({});
        ActiveCPUs[cpuId].scheduler->start();
    }

    Trampoline createTrampoline(BlockAllocator<SmallStack>& stackAllocator,
            BlockAllocator<VirtualMemoryManager>& vmmAllocator,
            int apicId,
            int cpuId) {

        auto stack = stackAllocator.allocate();
        auto vmm = vmmAllocator.allocate();
        InitialKernelVMM->cloneForUsermode(vmm, true);

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
            reinterpret_cast<uintptr_t>(stack) + sizeof(SmallStack) - sizeof(TrampolineStack)
        );
        *stackPointer = {
            reinterpret_cast<uintptr_t>(status),
            reinterpret_cast<uintptr_t>(applicationProcessorStartup),
            cpuId,
            apicId,
            reinterpret_cast<uintptr_t>(vmm)
        };

        auto data = reinterpret_cast<TrampolineData*>(cpuData);
        *data = {
            reinterpret_cast<uintptr_t>(gdtPointer),
            //vmm->getDirectoryPhysicalAddress(),
            Memory::InitialKernelVMM->getDirectoryPhysicalAddress(),
            reinterpret_cast<uintptr_t>(stackPointer)
        };

        return {data, stackPointer, status, savedPhysicalNextFreeAddress};
    }
    
    void replaceTrampolineStack(Trampoline& trampoline, BlockAllocator<SmallStack>& stackAllocator,
            BlockAllocator<VirtualMemoryManager>& vmmAllocator,
            int apicId,
            int cpuId) {

        auto stack = stackAllocator.allocate();
        auto vmm = vmmAllocator.allocate();
        InitialKernelVMM->cloneForUsermode(vmm, true);

        auto stackPointer = reinterpret_cast<TrampolineStack*>(
            reinterpret_cast<uintptr_t>(stack) + sizeof(SmallStack) - sizeof(TrampolineStack)
        );

        *stackPointer = {
            reinterpret_cast<uintptr_t>(trampoline.status),
            reinterpret_cast<uintptr_t>(applicationProcessorStartup),
            cpuId,
            apicId
        };
        
        trampoline.data->pageDirectoryAddress = vmm->getDirectoryPhysicalAddress();
        trampoline.data->stackAddress = reinterpret_cast<uintptr_t>(stackPointer);        
        *trampoline.status = 0;
    }

    int initializeApplicationProcessors(APIC::LocalAPICHeader* lapics, int numberOfAPs) {

        BlockAllocator<SmallStack> stackAllocator {InitialKernelVMM};
        BlockAllocator<VirtualMemoryManager> vmmAllocator {InitialKernelVMM};

        int validAPs = 0;
        auto trampoline = createTrampoline(stackAllocator, 
            vmmAllocator, 
            lapics[0].apicId,
            validAPs + 1);

        for (int i = 0; i < numberOfAPs; i++) {

            if (i > 0) {
                replaceTrampolineStack(trampoline, 
                    stackAllocator, 
                    vmmAllocator, 
                    lapics[i].apicId,
                    validAPs + 1);
            }

            APIC::sendInitIPI(lapics[i].apicId);
            kernel_sleep(10);
            APIC::sendStartupIPI(lapics[i].apicId, 0x3);
            kernel_sleep(1);

            //wait for up to 500ms
            int x = 50;

            while (*trampoline.status != 1 && x > 0) {
                kernel_sleep(10);
                x -= 1;
            }

            if (*trampoline.status != 1) {
                //this cpu failed to start
                continue;
            }

            validAPs++;

            *trampoline.status = 1337;

            while (*trampoline.status <= 9000) {
                kernel_sleep(1);
            }
        }

        /*
        TODO:

        free the physical page, use trampoline.savedPhysicalNextAddress
        */
       return validAPs;
    }

    Kernel::Scheduler* initialize(uint32_t kernelEndAddress) {

        initializeSSE();
        PIC::disable();

        asm volatile("sti");
        auto tss = createTSS((kernelEndAddress & ~0xfff) + 0x1000, 0xCFFF'F000, 0);
        loadTSS(0x28);
        KernelEndAddr = kernelEndAddress;

        if (auto acpiTable = CPU::parseACPITables()) {
            auto stats = APIC::countAPICStructures(acpiTable.value().apicStartAddress, acpiTable.value().apicTableLength);

            APIC::Allocators allocators {
                BlockAllocator<APIC::LocalAPICHeader> {Memory::InitialKernelVMM, stats.localAPICCount},
                BlockAllocator<APIC::IOAPICHeader> {Memory::InitialKernelVMM, stats.ioAPICCount},
                BlockAllocator<APIC::InterruptSourceOverride> {Memory::InitialKernelVMM, stats.interruptOverrideCount},
                BlockAllocator<APIC::LocalAPICNMI> {Memory::InitialKernelVMM, stats.nonMaskableInterruptCount}
            };

            auto structures = APIC::loadAPICStructures(*acpiTable, allocators);
            auto numberOfAPs = 0;
            APIC::setupISAIRQs(structures.ioHeaders[0]);
            writeModelSpecificRegister(ModelSpecificRegister::IA32_TSC_AUX, 0, 0);

            if (stats.localAPICCount > 1) {
                numberOfAPs = initializeApplicationProcessors(structures.localHeaders + 1, stats.localAPICCount - 1);

                for (int i = 0; i < numberOfAPs; i++) {
                    auto cpuId = i + 1;
                    auto addr = Memory::InitialKernelVMM->HACK_getNextAddress();
                    createTSS(addr, 0xCFFF'F000 - 0x1000 * cpuId, cpuId);

                }
            }

            BlockAllocator<TaskStore> storeAllocator {Memory::InitialKernelVMM, 1};
            storeAllocator.allocate();

            CurrentTaskLauncher = BlockAllocator<TaskLauncher>(Memory::InitialKernelVMM).allocate(tss);

            BlockAllocator<Scheduler> schedulerAllocator {Memory::InitialKernelVMM};

            auto scheduler = schedulerAllocator.allocate();
            setupCPUBlocks(1 + numberOfAPs, {scheduler});
                    //TODO: wrong
                    ActiveCPUs[1].apicId = structures.localHeaders[1].apicId;

            for (auto i = 0; i < numberOfAPs; i++) {
                ActiveCPUs[i + 1].scheduler = schedulerAllocator.allocate();
                Memory::InitialKernelVMM->activate();
            }

            CPUCount = 1 + numberOfAPs;

            //APIC::setupIOAPICs(structures, stats);
            APIC::setupAPICTimer();

            return scheduler;

        }
        else {

            kprintf("[Kernel] Parsing ACPI Tables failed, halting\n");
            asm volatile("hlt");
            return nullptr;
        }
    }

}
