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
#include <string.h>
#include <memory/physical_memory_manager.h>
#include <memory/virtual_memory_manager.h>
#include <system_calls.h>
#include <ipc.h>
#include <services.h>
#include <initialize_libc.h>
#include <services/terminal/vga.h>
#include <services/ps2/ps2.h>
#include <services/virtualFileSystem/virtualFileSystem.h>
#include <services/processFileSystem/processFileSystem.h>
#include <services/fakeFileSystem/fakeFileSystem.h>
#include <services/hardwareFileSystem/system.h>
#include <services/events/system.h>
#include <services/startup/startup.h>
#include <services/discovery/loader.h>
#include <cpu/initialize.h>
#include <cpu/cpu.h>
#include <scheduler.h>
#include <task.h>

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
    Memory::InitialKernelVMM = &virtualMemManager;

    Memory::InitialKernelVMM->preallocateKernelPageTables();

    GDT::setup();
    IDT::setup();
    VGA::disableCursor();
    PS2::disableController();

    auto scheduler = CPU::initialize(kernelEndAddress);    
    while (!CPU::ActiveCPUs[0].ready) {}

    //also don't need APIC tables anymore
    virtualMemManager.unmap(acpiStartAddress, acpiPages);

    kprintf("Saturn OS v 0.3.0\n------------------\n\nCalibrating clock, please wait...\n");

    ServiceRegistry registry;
    ServiceRegistryInstance = &registry;

    auto& launcher = *CurrentTaskLauncher;
    CPU::scheduleTask(launcher.createUserTask(reinterpret_cast<uint32_t>(VirtualFileSystem::service)));
    CPU::scheduleTask(launcher.createUserTask(reinterpret_cast<uint32_t>(PFS::service)));
    CPU::scheduleTask(launcher.createUserTask(reinterpret_cast<uint32_t>(FakeFileSystem::service)));
    CPU::scheduleTask(launcher.createKernelTask(reinterpret_cast<uint32_t>(Kernel::schedulerService)));
    CPU::scheduleTask(launcher.createUserTask(reinterpret_cast<uint32_t>(HardwareFileSystem::service)));
    CPU::scheduleTask(launcher.createKernelTask(reinterpret_cast<uint32_t>(HardwareFileSystem::detectHardware)));
    CPU::scheduleTask(launcher.createKernelTask(reinterpret_cast<uint32_t>(Discovery::discoverDevices)));
    CPU::scheduleTask(launcher.createUserTask(reinterpret_cast<uint32_t>(Event::service)));
    CPU::scheduleTask(launcher.createUserTask(reinterpret_cast<uint32_t>(Startup::service)));

    CPU::startScheduler();
    scheduler->start();

    return 0;
}
