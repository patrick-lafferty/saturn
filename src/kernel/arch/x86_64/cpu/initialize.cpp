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
#include "timers/pit.h"
#include "tss.h"
#include "halt.h"
#include "trampoline.h"
#include "idt/descriptor.h"
#include <gdt.h>

using namespace Memory;
extern "C" void initializeSSE();

namespace CPU {

    void kernel_sleep(int milliseconds) {
        RTC::enable(6);

        while (milliseconds > 0) {
            asm("hlt");
            milliseconds--;
            RTC::reset();
        }

        RTC::disable();
    }

    void applicationProcessorStartup(int cpuId, int apicId) {
        initializeSSE();
        GDT::load();
        IDT::initialize();

        loadTSSRegister(cpuId);

        halt("AP started");
    }

    void unmapACPITables(KernelConfig* config) {
        auto& core = getCurrentCore();

        uintptr_t address = config->acpiLocation;
        auto pages = config->acpiLength / 0x1000;

        for (auto i = 0u; i < pages; i++) {
            core.virtualMemory->unmap({address});
            address += 0x1000;
        }
    }

    void initializeApplicationProcessors(LinkedList<APIC::LocalAPICHeader, BlockAllocator>& headers,
            BlockAllocator<TSS>& tssAllocator) {
        auto& core = getCurrentCore();
        auto storage = core.physicalMemory->allocateRealModePage();
        storage = core.physicalMemory->allocateRealModePage();
        BlockAllocator<Trampoline::Stack> stackAllocator {AddressSpace::Domain::KernelStacks};
        int validAPs = 0;
        auto iterator = headers.getHead()->next;

        while (iterator != nullptr) {
            auto stack = stackAllocator.allocate();
            stack->data.kernelFuncAddress = reinterpret_cast<uintptr_t>(applicationProcessorStartup);
            stack->data.cpuId = validAPs + 1;
            stack->data.apicId = iterator->value.apicId;
            auto trampoline = Trampoline::create(stack, storage);
            auto readyFlag = reinterpret_cast<int*>(stack->data.cpuReadyFlagAddress);

            auto vmm = core.virtualMemory->clone();
            trampoline->data64Bit.pageDirectoryAddress = vmm.value().address;

            APIC::sendInitIPI(iterator->value.apicId);
            kernel_sleep(200);
            APIC::sendStartupIPI(iterator->value.apicId, storage.address / 0x1000);

            //wait for up to 500ms
            int x = 50;

            while (*readyFlag != 1 && x > 0) {
                kernel_sleep(10);
                x -= 1;
            }

            if (*readyFlag == 1) {
                validAPs++;
                *readyFlag = 1337;

                if (!setupTSS(tssAllocator)) {
                    halt("Could not setup an AP's TSS");
                }

                while (*readyFlag <= 9000) {
                    kernel_sleep(10);
                }
            }

            iterator = iterator->next;
        }
        
        halt("finished loading APs?");

        core.physicalMemory->freeRealModePage({storage.address});
    }

    void initialize(KernelConfig* config) {
        BlockAllocator<TSS> tssAllocator;

        setupTSS(tssAllocator);
        loadTSSRegister(0);

        BlockAllocator<APIC::Meta> apicMetaAllocator {1};
        auto& initialCore = getCurrentCore();
        initialCore.apic = apicMetaAllocator.allocate();

        if (auto acpiTable = CPU::parseACPITables(config->rsdpAddress)) {
            APIC::initialize();

            auto structures = APIC::loadAPICStructures(*acpiTable);
            initialCore.apic->localAPICAddress = structures.localAPICAddress;
            initialCore.apic->ioAPICAddress = structures.ioHeaders.getHead()->value.address;

            int startingAPICInterrupt = 32;
            APIC::setupIOAPICs(structures, startingAPICInterrupt);
            APIC::setupISAIRQs(startingAPICInterrupt);

            if (structures.localHeaders.getSize() > 1) {
                initializeApplicationProcessors(structures.localHeaders, tssAllocator);
            }

            RTC::enable(0xD);
            PIT::disable();
        }
        else {
            halt("Failed to parse the ACPI tables, halting.");
        }

        unmapACPITables(config);
    }
}