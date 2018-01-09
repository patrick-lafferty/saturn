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
#include "gdt.h"

GDT::Descriptor gdt[6];
GDT::DescriptorPointer gp;

namespace GDT {

    template<typename... Arg> uint32_t combineFlags(Arg... args) {
        return (static_cast<uint32_t>(args) | ...);
    }

    int nextGDTIndex = 0;

    void setup() {
        gp.limit = sizeof(Descriptor) * 6 - 1;
        gp.base = reinterpret_cast<uint32_t>(&gdt);

        //null descriptor
        gdt[0] = encodeEntry(0, 0, 0, 0);

        /*
        The upper 4 bits of flags is [Gr, Sz, 0, 0], where:
        Gr = Granularity (0 = Byte granularity, 1 = 4K Page granularity)
        Sz = Size (0 = 16 bit protected mode, 1 = 32 bit protected mode)

        Normally we want Gr = 1 and Sz = 1 for regular pages
        */
        
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

        //user code segment 
        gdt[3] = encodeEntry(0, 0xFFFFF, combineFlags(
            AccessCodeSegment::Present,
            AccessCodeSegment::Executable,
            AccessCodeSegment::Readable,
            AccessCodeSegment::Ring3
        ), combineFlags(Flags::Size, Flags::Granularity));
        //user data segment 
        gdt[4] = encodeEntry(0, 0xFFFFF, combineFlags(
            AccessDataSegment::Present,
            AccessDataSegment::Writeable,
            AccessDataSegment::Ring3
        ), combineFlags(Flags::Size, Flags::Granularity));
        
        nextGDTIndex = 5;
        
        gdt_flush();
    }

    Descriptor encodeEntry(uint32_t base, uint32_t limit, uint8_t access, uint8_t flags, bool setBit4) {
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

    void addTSSEntry(uint32_t address, uint32_t size) {
        //tss per cpu (hardware thread?)
        gdt[nextGDTIndex] = encodeEntry(address, size - 1, combineFlags(
            AccessCodeSegment::Accessed,
            AccessCodeSegment::Executable,
            AccessCodeSegment::Present
        ), combineFlags(Flags::Size), false);

        nextGDTIndex++;

        //TODO: is this necessary?
        gdt_flush();
    }
}