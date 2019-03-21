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
#include "trampoline.h"
#include <string.h>
#include "halt.h"
#include <gdt.h>

extern "C" void trampoline_entry();
extern "C" uint32_t trampoline_size;

namespace CPU::Trampoline {

    template<typename... Arg> uint32_t combineFlags(Arg... args) {
        return (static_cast<uint32_t>(args) | ...);
    }

    Descriptor encodeEntry(uint32_t base, uint32_t limit, uint8_t access, 
        uint8_t flags, bool setBit4 = true) {
        Descriptor descriptor;

        descriptor.baseLow = base & 0xFFFF;
        descriptor.baseMiddle = (base >> 16) & 0xFF;
        descriptor.baseHigh = (base >> 24) & 0xFF;

        descriptor.limitLow = limit & 0xFFFF;
        descriptor.flags = (limit >> 16) & 0x0F;
        descriptor.flags |= flags;
        descriptor.access = access;

        //bit 4 is always 1 for code/data segments, for tss its 0
        if (setBit4) {
            descriptor.access |= (1 << 4);
        }

        return descriptor;
    }

    void setupTemporaryGDT(Descriptor (&gdt)[3], DescriptorPointer& pointer) {
        pointer.limit = sizeof(Descriptor) * 3 - 1;
        pointer.base = reinterpret_cast<uintptr_t>(&gdt);

        //null descriptor
        gdt[0] = encodeEntry(0, 0, 0, 0);
        
        //kernel code segment 
        gdt[1] = encodeEntry(0, 0xFFFFF, combineFlags(
            AccessCodeSegment::Present,
            AccessCodeSegment::Executable,
            AccessCodeSegment::Readable
        ), combineFlags(Flags::Size, Flags::Granularity));
        //kernel data segment 
        gdt[2] = encodeEntry(0, 0xFFFFF, combineFlags(
            AccessDataSegment::Present,
            AccessDataSegment::Writeable
        ), combineFlags(Flags::Size, Flags::Granularity));
    }

    void setupTemporaryGDT64(uint64_t (&gdt)[3], GDT::DescriptorPointer<uint64_t>& pointer) {
        pointer.limit = sizeof(uint64_t) * 3 - 1;
        pointer.base = reinterpret_cast<uintptr_t>(&gdt);
        /* 
        These values are based off of the tables in AMD's
        "AMD64 Architecture Programmer's Manual Volume 2",
        pages 88-89
        */
        gdt[0] = 0;
        gdt[1] = (0b00000000'00100000'10011000'00000000ull) << 32;
        //Even though AMD says W bit is ignored, Intel requires it
        gdt[2] = (0b00000000'00000000'10010010'00000000ull) << 32;
    }

    Trampoline* create(Stack* stack, Memory::PhysicalAddress storageAddress) {
        auto storage = reinterpret_cast<uint8_t*>(storageAddress.address);

        const int requiredSpace = sizeof(Trampoline) + sizeof(Stack) + trampoline_size;
        const int PageSize = 0x1000;

        if (requiredSpace >= PageSize) {
            halt("Trampoline can't fit inside a single page");
        }

        /*
        Layout of the trampoline page:

        0               ... trampoline_size: executable code for trampoline
        trampoline_size ... + 3 * sizeof Descriptor: 3 32-bit GDT entries
                        ... + 3 * sizeof Descriptor64: 3 64-bit GDT entries
                        ... + 4: status flag
        (4096 - sizeof Trampoline - 8): cpu ready flag
        remaining: Trampoline struct
        */

        memcpy(storage, reinterpret_cast<void*>(trampoline_entry), trampoline_size);

        /*
        Patch the trampoline code to replace the fixed 0x4000 jump
        offsets with the actual address of storage
        */
        uint8_t* code = storage;
        int patchesApplied = 0;
        for (int i = 0; i < trampoline_size; i++) {
            if (*code == 0xea) {
                if (*(code + 3) == 0x08) {
                    //this is the first jump to patch
                    *(code + 2) = storageAddress.address / 0x100;
                    patchesApplied++;
                }
            }
            else if (*code == 0x0f) {
                if ((*(code + 1) == 0x01)
                    && (*(code + 2) == 0x10)
                    && (*(code + 3) == 0xea)) {
                    //this is the second jump to patch
                    *(code + 5) = storageAddress.address / 0x100;
                    patchesApplied++;
                    break;
                }
            }

            code++;
        }

        if (patchesApplied != 2) {
            halt("Failed to patch trampoline code");
        }

        storage += trampoline_size;

        typedef Descriptor Descriptors[3];
        Descriptors* gdt32 = reinterpret_cast<Descriptors*>(storage);
        storage += sizeof(Descriptor) * 3;
        auto gdtPointer = reinterpret_cast<DescriptorPointer*>(storage);
        storage += sizeof(DescriptorPointer);

        setupTemporaryGDT(*gdt32, *gdtPointer);

        typedef uint64_t Descriptors64[3];
        Descriptors64* gdt64 = reinterpret_cast<Descriptors64*>(storage);
        storage += sizeof(uint64_t) * 3;
        auto gdt64Pointer = reinterpret_cast<GDT::DescriptorPointer<uint64_t>*>(storage);
        storage += sizeof(GDT::DescriptorPointer<uint64_t>);

        setupTemporaryGDT64(*gdt64, *gdt64Pointer);

        stack->cpuReadyFlagAddress = storageAddress.address + (PageSize - sizeof(Trampoline)) - 8;

        auto trampoline = reinterpret_cast<Trampoline*>(storageAddress.address + (PageSize - sizeof(Trampoline)));
        trampoline->data32Bit.gdtPointerAddress = reinterpret_cast<uintptr_t>(gdtPointer);
        trampoline->data64Bit.gdtPointerAddress = reinterpret_cast<uintptr_t>(gdt64Pointer);
        trampoline->stackAddress = reinterpret_cast<uintptr_t>(stack);

        return trampoline;
    }
}