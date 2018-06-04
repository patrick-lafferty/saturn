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
#include <stdio.h>
#include <gdt/gdt.h>
#include <idt/idt.h>
#include <cpu/pic.h>
#include <cpu/acpi.h>
#include <cpu/sse.h>
#include <cpu/tss.h>
#include <cpu/cpu.h>
#include <string.h>
#include <memory/physical_memory_manager.h>
#include <memory/virtual_memory_manager.h>
#include <scheduler.h>
#include <system_calls.h>
#include <test/libc/runner.h>
#include <test/libc++/new.h>
#include <test/libc++/list.h>
#include <ipc.h>
#include <services.h>
#include <initialize_libc.h>
#include <heap.h>
#include <task.h>
#include <services/memory/memory.h>
#include <services/terminal/vga.h>
#include <services/virtualFileSystem/virtualFileSystem.h>
#include <services/processFileSystem/processFileSystem.h>
#include <services/fakeFileSystem/fakeFileSystem.h>
#include <services/hardwareFileSystem/system.h>
#include <services/events/system.h>
#include <services/startup/startup.h>
#include <services/discovery/loader.h>

#include <cpu/apic.h>

using namespace Kernel;
using namespace Memory;

extern uint32_t __kernel_memory_start;
extern uint32_t __kernel_memory_end;

/*
We get a pointer to this struct passed in from boot.s to kernel_main.
The address is to an instance of the struct created in the pre-kernel,
in pre_kernel.cpp
*/
struct MemManagerAddresses {
    uint32_t physicalManager;
    uint32_t virtualManager;
    uint32_t multiboot;
    uint32_t acpiStartAddress;
    uint32_t acpiPages;
};

void infiniteLoop() {
    while (true) {
        int x = 0;
        x++;
    }
}

extern "C" void trampoline_end();
extern "C" void trampoline_start();
extern "C" uint8_t trampoline_size;

void setupCPU() {

    BlockAllocator<Stack> stackAllocator {currentVMM};
    BlockAllocator<VirtualMemoryManager> vmmAllocator {currentVMM};

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

    auto pt = reinterpret_cast<GDT::DescriptorPointer*>(config);
    pt->limit = sizeof(GDT::Descriptor) * 6 - 1;
    pt->base = startingGDTAddress;

    auto cpuData = ((int*)configMemoryBlock) + 0x3Fb;
    *cpuData = 0;
    auto cpuReadyFlag = cpuData++;

    *cpuData = reinterpret_cast<uintptr_t>(pt);
    *++cpuData = vmm->getDirectoryPhysicalAddress();
    *++cpuData = reinterpret_cast<uintptr_t>(stack) + sizeof(Stack) - 8;
    auto stackPointer = reinterpret_cast<uint32_t*>(*cpuData);
    *stackPointer++ = reinterpret_cast<uintptr_t>(cpuReadyFlag);
    *stackPointer = reinterpret_cast<uintptr_t>(infiniteLoop);

    APIC::sendInitIPI(1);
    uint32_t x = 10000;
    while (x > 0) x--;
    APIC::sendStartupIPI(1, 0x3);
    x = 10000;
    while (x > 0) x--;

    while (*cpuReadyFlag != 1) {}
    *cpuReadyFlag = 1337;

    while (*cpuReadyFlag <= 9000) {}
    
    while (true) {}
}

extern "C" int kernel_main(MemManagerAddresses* addresses) {

    initializeLibC();

    auto acpiStartAddress = addresses->acpiStartAddress;
    auto acpiPages = addresses->acpiPages;
    /*
    The prekernel creates its own PMM and VMM in lower addresses.
    We want the higher-half kernel to have its own proper versions
    so we can unmap the old addresses, so copy them over here.
    */
    PhysicalMemoryManager physicalMemManager = 
        *reinterpret_cast<PhysicalMemoryManager*>(
            addresses->physicalManager);

    VirtualMemoryManager virtualMemManager = 
        *reinterpret_cast<VirtualMemoryManager*>(
            addresses->virtualManager);

    virtualMemManager.setPhysicalManager(&physicalMemManager);

    auto kernelStartAddress = reinterpret_cast<uint32_t>(&__kernel_memory_start);
    auto kernelEndAddress = reinterpret_cast<uint32_t>(&__kernel_memory_end);
    const uint32_t virtualOffset = 0xD000'0000;

    //we don't need the identity map anymore
    virtualMemManager.unmap(kernelStartAddress, 1 + (kernelEndAddress - virtualOffset - kernelStartAddress) / 0x1000);

    Memory::currentPMM = &physicalMemManager;
    Memory::currentVMM = &virtualMemManager;

    Memory::currentVMM->preallocateKernelPageTables();

    GDT::setup();
    IDT::setup();
    initializeSSE();
    PIC::disable();

    VGA::disableCursor();

    asm volatile("sti");

    auto pageFlags = static_cast<int>(PageTableFlags::AllowWrite);
    auto afterKernel = (kernelEndAddress & ~0xfff) + 0x1000;

    virtualMemManager.HACK_setNextAddress(0xCFFF'F000);
    auto tssAddress = virtualMemManager.allocatePages(1, pageFlags);
    virtualMemManager.HACK_setNextAddress(afterKernel);

    HACK_TSS_ADDRESS = tssAddress;
    GDT::addTSSEntry(tssAddress, 0x1000 * 1);
    auto tss = CPU::setupTSS(tssAddress);

    if (!CPU::parseACPITables()) {

        kprintf("[Kernel] Parsing ACPI Tables failed, halting\n");
        asm volatile("hlt");
    }

    setupCPU();

    //also don't need APIC tables anymore
    //NOTE: if we actually do, copy them before this line to a new address space
    virtualMemManager.unmap(acpiStartAddress, acpiPages);

    LibC_Implementation::createHeap(PageSize * PageSize * 10, &virtualMemManager);

    Kernel::TaskLauncher launcher {tss};
    Kernel::Scheduler scheduler;
    CPU::setupCPUBlocks(1, {&scheduler});

    kprintf("Saturn OS v 0.3.0\n------------------\n\nCalibrating clock, please wait...\n");

    ServiceRegistry registry;
    ServiceRegistryInstance = &registry;

    scheduler.scheduleTask(launcher.createUserTask(reinterpret_cast<uint32_t>(VirtualFileSystem::service)));
    scheduler.scheduleTask(launcher.createUserTask(reinterpret_cast<uint32_t>(PFS::service)));
    scheduler.scheduleTask(launcher.createUserTask(reinterpret_cast<uint32_t>(FakeFileSystem::service)));
    scheduler.scheduleTask(launcher.createUserTask(reinterpret_cast<uint32_t>(HardwareFileSystem::service)));
    scheduler.scheduleTask(launcher.createKernelTask(reinterpret_cast<uint32_t>(HardwareFileSystem::detectHardware)));
    scheduler.scheduleTask(launcher.createKernelTask(reinterpret_cast<uint32_t>(Discovery::discoverDevices)));
    scheduler.scheduleTask(launcher.createUserTask(reinterpret_cast<uint32_t>(Event::service)));
    scheduler.scheduleTask(launcher.createUserTask(reinterpret_cast<uint32_t>(Startup::service)));
    
    scheduler.enterIdle();

    return 0;
}
