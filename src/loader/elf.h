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

namespace Elf {

    struct Header {
        uint8_t identification[16];
        uint16_t objectFileType;
        uint16_t machineType;
        uint32_t fileVersion;
        uint64_t entryPoint;
        uint64_t programHeaderOffset;
        uint64_t sectionHeaderOffset;
        uint32_t processorFlags;
        uint16_t programHeaderEntrySize;
        uint16_t programHeaderEntryCount;
        uint16_t sectionHeaderEntrySize;
        uint16_t sectionHeaderEntryCount;
        uint16_t sectionNameTableIndex;
    };

    enum class IdentifierBits {
        Magic0,
        Magic1,
        Magic2,
        Magic3,
        Class,
        DataEncoding,
        Version,
        OSABI,
        ABIVersion
    };

    enum class Class {
        Class32 = 1,
        Class64 = 2
    };

    enum class DataEncoding {
        LittleEndian = 1,
        BigEndian = 2
    };

    enum class Type {
        None,
        RelocatableObject,
        Executable,
        SharedObject,
        CoreFile,
        LOOS = 0xFE00,
        HIOS = 0xFEFF,
        LOPROC = 0xFF00,
        HIPROC = 0xFFFF
    };

    struct ProgramHeader {
        uint32_t type;
        uint32_t flags;
        uint64_t offset;
        uint64_t virtualAddress;
        uint64_t physicalAddress;
        uint64_t fileSize;
        uint64_t memorySize;
        uint64_t alignment;
    };

    enum class ProgramType {
        Null = 0,
        Load = 1,
        Dynamic = 2,
        InterpreterPathName = 3,
        Note = 4,
        SHLIB = 5,
        ProgramHeaderTable = 6,
        LOOS = 0x6000'0000,
        HIOS = 0x6fff'ffff,
        LOPROC = 0x7000'0000,
        HIPROC = 0x7fff'ffff
    };

    struct Program {
        uint64_t sourcePhysicalAddress;
        uint64_t destinationVirtualAddress;
        uint64_t length;
        uint64_t entryPoint;
    };

    bool verifyHeader(Header* header);
    bool loadElfExecutable(uintptr_t address, Program& program);
}