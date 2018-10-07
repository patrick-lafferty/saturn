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
#include <stdint.h>

void printString(const char* message, int line, int column) {

    auto position = line * 80 + column;
    auto buffer = reinterpret_cast<uint16_t*>(0xb8000) + position;

    while(message && *message != '\0') {
        *buffer++ = *message++ | (0xF << 8);
    }
}

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
#include <gdt.h>

using namespace Memory;

extern "C" void initializeSSE();

extern "C"
[[noreturn]]
void initializeKernel(uint64_t firstFreeAddress, uint64_t totalFreePages) {

    GDT::setup();

    Memory::PhysicalMemoryManager::SetupGlobalManager(firstFreeAddress, totalFreePages);
    auto& physicalMemoryManager = PhysicalMemoryManager::GetGlobalManager();
    /*VirtualMemoryManager virtualMemoryManager;
    virtualMemoryManager.map(0x6969696969, 0x303030);*/
    initializeSSE();
    IDT::initialize();
    PIC::disable();
    asm("sti");

    //Memory::BlockAllocator<int> alloc;

    printString("Inside kernel", 0, 0);
    asm("int $0");
    halt();
}
