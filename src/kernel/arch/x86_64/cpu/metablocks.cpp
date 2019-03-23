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
#include "metablocks.h"
#include "msr.h"
#include <memory/address_space.h>

namespace CPU {

    CoreMeta InitialCore;
    CoreMeta* Cores;

    void storeCore(CoreMeta* core) {
        auto address = reinterpret_cast<uintptr_t>(core);

        writeModelSpecificRegister(ModelSpecificRegister::GSBase, 
            address & 0xFFFFFFFF, address >> 32);
        writeModelSpecificRegister(ModelSpecificRegister::KernelGSBase, 
            address & 0xFFFFFFFF, address >> 32);
    }

    void setupInitialCore(Memory::PhysicalMemoryManager* physicalMemory,
            Memory::VirtualMemoryManager* virtualMemory) {
        
        InitialCore.self = &InitialCore;
        InitialCore.physicalMemory = physicalMemory;
        InitialCore.virtualMemory = virtualMemory;

        storeCore(&InitialCore);

        using namespace Memory;

        static AddressSpace space;
        static AddressSpace stackSpace {{PDPSize * 511ul + 0x200000ul}, 0x200000, AddressSpace::Domain::KernelStacks};
        InitialCore.addressSpaces[static_cast<int>(AddressSpace::Domain::Default)] = &space;
        InitialCore.addressSpaces[static_cast<int>(AddressSpace::Domain::KernelStacks)] = &stackSpace;
    }

    void setupCore(Memory::PhysicalMemoryManager* physicalMemory,
            Memory::VirtualMemoryManager* virtualMemory,
            Memory::AddressSpace* addressSpace) {
        static CoreMeta core;
        core.self = &core;
        core.physicalMemory = physicalMemory;
        core.virtualMemory = virtualMemory;
        core.addressSpaces[static_cast<int>(Memory::AddressSpace::Domain::Default)] = addressSpace;

        storeCore(&core);
    }

    CoreMeta& getCurrentCore() {

        CoreMeta* pointer {nullptr};

        asm("mov %%gs:[0], %0"
            : "=r" (pointer));

        return *pointer;
    }
}
