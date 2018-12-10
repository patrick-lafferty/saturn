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
#include "acpi.h"
#include <misc/kernel_initial_arguments.h>
#include "metablocks.h"
#include <memory/virtual_memory_manager.h>
#include <memory/block_allocator.h>
#include "apic.h"
#include "timers/rtc.h"
#include "tss.h"

using namespace Memory;

namespace CPU {

    void unmapACPITables(KernelConfig* config) {
        auto& core = getCurrentCore();

        uintptr_t address = config->acpiLocation;
        auto pages = config->acpiLength / 0x1000;

        for (auto i = 0u; i < pages; i++) {
            core.virtualMemory->unmap(address, 1);
            address += 0x1000;
        }
    }

    void initialize(KernelConfig* config) {
        BlockAllocator<TSS> tssAllocator {1};

        setupTSS(tssAllocator);
        loadTSSRegister();

        if (auto acpiTable = CPU::parseACPITables(config->rsdpAddress)) {
            APIC::initialize();

            APIC::Allocators allocators {
                BlockAllocator<APIC::LocalAPICHeader> {10},
                BlockAllocator<APIC::IOAPICHeader> {10},
                BlockAllocator<APIC::InterruptSourceOverride> {10},
                BlockAllocator<APIC::LocalAPICNMI> {10}
            };

            auto structures = APIC::loadAPICStructures(*acpiTable, allocators);
            APIC::setupISAIRQs(structures.ioHeaders[0]);

            RTC::enable(0xD);
        }

        unmapACPITables(config);
    }
}