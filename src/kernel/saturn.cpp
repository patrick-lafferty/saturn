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
[[noreturn]]
void halt() {
    while (true) {
        asm("cli \n"
            "hlt");
    }
}

#include <memory/physical_memory_manager.h>
#include <memory/block_allocator.h>
#include <idt/descriptor.h>
#include <cpu/pic.h>
#include <cpu/metablocks.h>
#include <gdt.h>
#include <log.h>

using namespace Memory;

extern "C" void initializeSSE();

class Test {
public:

    Test() {
        x = 1336;
    }

    static void xx() {
        y++;
    }

    static int y;
    int x;
};

class Test2 {
public:

    Test2() {
        x = 2336;
    }

    static void xx() {
        y++;
    }

    static int y;
    int x;
};

int Test::y = 5;

static Test test;
static Test2 test2;

extern "C"
[[noreturn]]
void initializeKernel(uint64_t firstFreeAddress, uint64_t totalFreePages) {

    GDT::setup();

    Memory::PhysicalMemoryManager::SetupGlobalManager(firstFreeAddress, totalFreePages);
    auto& physicalMemoryManager = PhysicalMemoryManager::GetGlobalManager();
    initializeSSE();
    IDT::initialize();
    PIC::disable();
    asm("sti");

    //Memory::BlockAllocator<int> alloc;
    VirtualMemoryManager virtualMemoryManager;

    CPU::setupCore(&physicalMemoryManager, &virtualMemoryManager);

    //virtualMemoryManager.map(0x6969696969, 0x303030);
    int x = *reinterpret_cast<int*>(0xabcdef00);

    log("hello %d and %d", 1, 1);
    log("hello %d and %d", 2, 3);
    log("hello %d and %d", 5, 8);

    halt();
}
