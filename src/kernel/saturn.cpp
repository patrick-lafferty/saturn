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

#include <memory/physical_memory_manager.h>
#include <memory/block_allocator.h>
#include <idt/descriptor.h>
#include <cpu/pic.h>
#include <cpu/metablocks.h>
#include <cpu/initialize.h>
#include <cpu/halt.h>
#include <gdt.h>
#include <log.h>
#include <misc/kernel_initial_arguments.h>

using namespace Memory;

extern "C" void initializeSSE();

#include "../../test/kernel/kernel.h"

extern "C"
[[noreturn]]
void initializeKernel(KernelConfig* config) {
    PIC::disable();
    GDT::setup();
    Memory::PhysicalMemoryManager::SetupGlobalManager(config->nextFreeAddress, config->totalFreePages);
    auto& physicalMemoryManager = PhysicalMemoryManager::GetGlobalManager();

    initializeSSE();
    IDT::initialize();
    asm("sti");

    VirtualMemoryManager virtualMemoryManager;

    auto p1 = physicalMemoryManager.allocateDMAPage();
    auto p2 = physicalMemoryManager.allocateDMAPage();
    auto p3 = physicalMemoryManager.allocateDMAPage();

    physicalMemoryManager.freeDMAPage(p2);

    CPU::setupInitialCore(&physicalMemoryManager, &virtualMemoryManager);

  //  Test::runKernelTests();
    CPU::initialize(config);

    log("Saturn OS 0.4");


    halt("Todo: add OS here");
}
