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
#pragma once

#include <stdint.h>

namespace GDT {

    struct Descriptor {
        uint16_t limitLow;
        uint16_t baseLow;
        uint8_t baseMiddle;
        uint8_t access;
        uint8_t flags;
        uint8_t baseHigh;
    } __attribute__((packed));

    enum class AccessDataSegment {
        Accessed = 1 << 0,
        Writeable = 1 << 1,
        Direction = 1 << 2,
        Ring3 = 0x60,
        Present = 1 << 7
    };

    enum class AccessCodeSegment {
        Accessed = 1 << 0,
        Readable = 1 << 1,
        Conforming = 1 << 2,
        Executable = 1 << 3,
        Ring3 = 0x60,
        Present = 1 << 7
    };

    enum class Flags {
        LongBit = 1 << 5,
        Size = 1 << 6,
        Granularity = 1 << 7
    };

    template<typename AddressSize>
    struct DescriptorPointer {
        uint16_t limit;
        AddressSize base;
    } __attribute__((packed));

    void setup32();
    void setup64();

    void addTSSEntry(uint32_t address, uint32_t size);
}

extern "C" GDT::Descriptor gdt32[3];
extern "C" GDT::DescriptorPointer<uint32_t> gp32;
extern "C" uint64_t gdt64[3];
extern "C" GDT::DescriptorPointer<uint64_t> gp64;

extern "C" void gdt32_flush();