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
#include "gdt.h"

GDT::Descriptor gdt32[3];
GDT::DescriptorPointer<uint32_t> gp32;
uint64_t gdt64[3];
GDT::DescriptorPointer<uint64_t> gp64;

namespace GDT {

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

    int nextGDTIndex = 0;

    void setup32() {
        gp32.limit = sizeof(Descriptor) * 3 - 1;
        gp32.base = reinterpret_cast<uint32_t>(&gdt32);

        //null descriptor
        gdt32[0] = encodeEntry(0, 0, 0, 0);
        
        //kernel code segment 
        gdt32[1] = encodeEntry(0, 0xFFFFF, combineFlags(
            AccessCodeSegment::Present,
            AccessCodeSegment::Executable,
            AccessCodeSegment::Readable
        ), combineFlags(Flags::Size, Flags::Granularity));
        //kernel data segment 
        gdt32[2] = encodeEntry(0, 0xFFFFF, combineFlags(
            AccessDataSegment::Present,
            AccessDataSegment::Writeable
        ), combineFlags(Flags::Size, Flags::Granularity));

        nextGDTIndex = 3;
        
        gdt32_flush();
    }

    void setup64() {

        gp64.limit = 8 * 3 - 1;
        gp64.base = reinterpret_cast<uint64_t>(&gdt64[0]);

        /* 
        These values are based off of the tables in AMD's
        "AMD64 Architecture Programmer's Manual Volume 2",
        pages 88-89
        */
        gdt64[0] = 0;
        gdt64[1] = (0b00000000'00100000'10011000'00000000ull) << 32;
        gdt64[2] = (0b00000000'00000000'10000000'00000000ull) << 32;

        nextGDTIndex = 3;
    }

    void addTSSEntry(uint32_t address, uint32_t size) {

        gdt32[nextGDTIndex] = encodeEntry(address, size - 1, combineFlags(
            AccessCodeSegment::Accessed,
            AccessCodeSegment::Executable,
            AccessCodeSegment::Present
        ), combineFlags(Flags::Size), false);

        nextGDTIndex++;
    }
}