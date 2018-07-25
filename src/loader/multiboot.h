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

namespace Multiboot {

    struct BootInfo {
        uint32_t totalSize;
        uint32_t reserved;
    };

    struct Tag {
        uint32_t type;
        uint32_t size;
    };

    enum class TagTypes {
        End = 0,
        BootCommandLine = 1,
        BootLoaderName = 2,
        Modules = 3,
        BasicMemory = 4,
        BIOSBootDevice = 5,
        MemoryMap = 6,
        VBEInfo = 7,
        FramebufferInfo = 8,
        ELFSymbols = 9,
        APMTable = 10,
        EFI32Table = 11,
        EFI64Table = 12,
        SMBIOSTable = 13,
        OldRDSP = 14,
        NewRDSP = 15,
        NetworkingInfo = 16,
        EFIMemoryMap = 17,
    };

    struct BootCommandLine : Tag {
        uint8_t* commandLine;
    };

    struct BootLoaderName : Tag {
        uint8_t* name;
    };

    struct Modules : Tag {
        uint32_t start;
        uint32_t end;
        uint8_t moduleStringStart;
    };

    struct BasicMemoryInfo : Tag {
        uint32_t lowerAmount;
        uint32_t upperAmount;
    };

    struct BiosBootDevice : Tag {
        uint32_t driveNumber;
        uint32_t partition;
        uint32_t subPartition;
    };

    struct MemoryMapEntry {
        uint64_t baseAddress;
        uint64_t length;
        uint32_t type;
        uint32_t reserved;
    } __attribute__((packed));
    
    struct MemoryMap : Tag {
        uint32_t entrySize;
        uint32_t version;
        uintptr_t entries;
    };

    struct VBEInfo : Tag {
        uint16_t mode;
        uint16_t interfaceSegment;
        uint16_t interfaceOffset;
        uint16_t interfaceLength;
        uint8_t controlInfo[512];
        uint8_t modeInfo[256];
    };

    struct Palette {
        uint8_t red;
        uint8_t green;
        uint8_t blue;
    };

    struct ColourInfo {
        uint32_t paletteColours;
        Palette* descriptors;
    };

    struct DirectColourInfo {
        uint8_t redFieldPosition;
        uint8_t redMaskSize;
        uint8_t greenFieldPosition;
        uint8_t greenMaskSize;
        uint8_t blueFieldPosition;
        uint8_t blueMaskSize;
    };

    struct FramebufferInfo : Tag {
        uint64_t address;
        uint32_t pitch;
        uint32_t width;
        uint32_t height;
        uint8_t bitsPerPixel;
        uint8_t type;
        uint8_t reserved;

        union {
            ColourInfo colourInfo;
            DirectColourInfo directRGB;
        };
    };

    struct ElfSymbols : Tag {
        uint16_t numberOfEntries;
        uint16_t entrySize;
        uint16_t index;
        uint16_t reserved;
        uint8_t* sectionHeadersUNUSED;
    };

    struct OldRDSP : Tag {

    };
}
