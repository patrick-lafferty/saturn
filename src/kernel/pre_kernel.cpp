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
#define TARGET_PREKERNEL true
#include <memory/physical_memory_manager.h>
#include <memory/virtual_memory_manager.h>

using namespace Kernel;
using namespace MemoryPrekernel;

extern uint32_t __kernel_memory_start;
extern uint32_t __kernel_memory_end;

__attribute__((section(".setup")))
PhysicalMemoryManager _physicalMemManager;
__attribute__((section(".setup")))
VirtualMemoryManager _virtualMemManager;

struct MemManagerAddresses {
    uint32_t physicalManager;
    uint32_t virtualManager;
    uint32_t multiboot;
    uint32_t acpiStartAddress;
    uint32_t acpiPages;
};

extern "C" 
__attribute__((section(".setup")))
MemManagerAddresses MemoryManagerAddresses;
__attribute__((section(".setup")))
MemManagerAddresses MemoryManagerAddresses;

const uint32_t virtualOffset = 0xD000'0000;

extern "C" void setupKernel(MultibootInformation* info) {
    auto acpiTableLocation = _physicalMemManager.initialize(info);
    _virtualMemManager.initialize(&_physicalMemManager);

    MemoryManagerAddresses.physicalManager = reinterpret_cast<uint32_t>(&_physicalMemManager);
    MemoryManagerAddresses.virtualManager = reinterpret_cast<uint32_t>(&_virtualMemManager);
    MemoryManagerAddresses.multiboot = reinterpret_cast<uint32_t>(info);

    auto pageFlags = 
        static_cast<int>(PageTableFlags::Present)
        | static_cast<int>(PageTableFlags::AllowWrite);

    //VGA video memory
    _virtualMemManager.map_unpaged(0xB8000 + virtualOffset, 0xB8000, 1, pageFlags);

    //the first megabyte of memory, for bios stuff
    _virtualMemManager.map_unpaged(virtualOffset, 0, 0x100000 / 0x1000, pageFlags);

    _virtualMemManager.map_unpaged(0x3000, 0x3000, 2, pageFlags);

    auto kernelStartAddress = reinterpret_cast<uint32_t>(&__kernel_memory_start);
    auto kernelEndAddress = reinterpret_cast<uint32_t>(&__kernel_memory_end);

    //TODO: until we get an elf loader, kernel code needs to have usermode access
    pageFlags |= static_cast<int>(PageTableFlags::AllowUserModeAccess); 

    //the higher-half kernel address
    _virtualMemManager.map_unpaged(kernelStartAddress + virtualOffset, kernelStartAddress, 1 + (kernelEndAddress - virtualOffset - kernelStartAddress) / 0x1000, pageFlags);
    
    /*
    temporarily identity map the kernel until we enter the higher half, so EIP
    is always valid
    */
    _virtualMemManager.map_unpaged(kernelStartAddress, kernelStartAddress , 1 + (kernelEndAddress - virtualOffset - kernelStartAddress) / 0x1000, pageFlags);

    //TODO: until we get an elf loader, kernel code needs to have usermode access
    pageFlags &= ~static_cast<int>(PageTableFlags::AllowUserModeAccess); 

    ACPITableLocation location {static_cast<uint32_t>(acpiTableLocation >> 32), 
        static_cast<uint32_t>(acpiTableLocation & 0xFFFFFFFF)};

    //ACPI tables
    if (location.startAddress == 0 || location.pages == 0) {
        //QEMU doesn't record the ACPI memory range as type 3, so just hardcode it
        _virtualMemManager.map_unpaged(0x7fe0000, 0x7fe0000, (0x8fe0000 - 0x7fe0000) / 0x1000, pageFlags);
    }
    else {
        //we're on a sane system, map the proper addresses
        _virtualMemManager.map_unpaged(location.startAddress, location.startAddress, location.pages, pageFlags);
    }

    MemoryManagerAddresses.acpiStartAddress = location.startAddress;
    MemoryManagerAddresses.acpiPages = location.pages;

    //APIC registers
    _virtualMemManager.map_unpaged(0xfec00000, 0xfec00000, (0xfef00000 - 0xfec00000) / 0x1000, pageFlags | 0b10000);

    _virtualMemManager.activate();
}