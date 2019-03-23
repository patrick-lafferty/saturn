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

uint64_t Gdt[GDT::MaxEntries];
GDT::DescriptorPointer<uint64_t> GdtPointer;

extern "C" void gdt_flush();

namespace GDT {

    template<typename... Arg> uint32_t combineFlags(Arg... args) {
        return (static_cast<uint32_t>(args) | ...);
    }

    int nextGDTIndex = 0;

    void setup() {

        GdtPointer.limit = 8 * MaxEntries - 1;
        GdtPointer.base = reinterpret_cast<uint64_t>(&Gdt[0]);

        /* 
        These values are based off of the tables in AMD's
        "AMD64 Architecture Programmer's Manual Volume 2",
        pages 88-89
        */
        Gdt[0] = 0;
        Gdt[1] = (0b00000000'00100000'10011000'00000000ull) << 32;
        //Even though AMD says W bit is ignored, Intel requires it
        Gdt[2] = (0b00000000'00000000'10010010'00000000ull) << 32;

        nextGDTIndex = 3;
        load();
    }

    void load() {

        gdt_flush();
    }

    struct Descriptor {
        uint16_t limitLow;
        uint16_t baseLow;
        uint8_t baseMiddle;
        uint8_t access;
        uint8_t flags;
        uint8_t baseHigh;
    } __attribute__((packed));

    bool addTSSEntry(uintptr_t address, uint32_t size) {

        if (nextGDTIndex > (MaxEntries - 2)) {
            return false;
        }

        /*
        In long mode TSS entries are stored as two consecutive
        GDT entries, with the format:

        first entry:

        bit 63-bit56     | 0's.. bit 47 | 0's.. bit 43-bit 40 | bits 39 - 16         | bits 15 - 0
        4th byte of base |             1               1001   |  first 3 addr bytes  | first 2 bytes of limit
        */
        uint64_t firstEntry = ((address & 0xFFFFFF) << 16) | (size & 0xFFFF);
        firstEntry |= (1ul << 47);
        firstEntry |= (0b1001ul << 40);

        uint64_t secondEntry = address >> 32;

        Gdt[nextGDTIndex++] = firstEntry;
        Gdt[nextGDTIndex++] = secondEntry;

        return true;
    }

}
