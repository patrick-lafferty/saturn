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

struct ElfHeader {
    uint32_t magicNumber;
    uint8_t bitFormat;
    uint8_t endianness;
    uint8_t versionShort;
    uint8_t osABI;
    uint8_t abiVersion;
    uint8_t pad[7];
    uint16_t type;
    uint16_t machine;
    uint32_t versionLong;
    uint32_t entryPoint;
    uint32_t programHeaderStart;
    uint32_t sectionHeaderStart;
    uint32_t flags;
    uint16_t headerSize;
    uint16_t programHeaderEntrySize;
    uint16_t programHeaderEntryCount;
    uint16_t sectionHeaderEntrySize;
    uint16_t sectionHeaderEntryCount;
    uint16_t sectionHeaderNameIndex;
};

struct ProgramHeader {
    uint32_t type;
    uint32_t offset;
    uint32_t virtualAddress;
    uint32_t physicalAddress;
    uint32_t imageSize;
    uint32_t memorySize;
    uint32_t flags;
    uint32_t alignment;
};

extern "C" uint32_t loadElf(char* /*pah*/) {
    const char* path = "/applications/test/test.bin";
    auto file = fopen(path, "");
#if 0
    char* data = new char[0x2000];

    fread(data, 0x2000, 1, file);

    return 0;
#endif

#if 1
    ElfHeader header;
    fread(&header, sizeof(header), 1, file);
    fseek(file, header.programHeaderStart, SEEK_SET);
    ProgramHeader programs[3];
    ProgramHeader program;

    for (auto i = 0u; i < header.programHeaderEntryCount; i++) {
        fread(&programs[i], sizeof(program), 1, file);
    }

    program = programs[0]; 

    /*auto savedAddress = Memory::currentVMM->HACK_getNextAddress();

    Memory::currentVMM->HACK_setNextAddress(program.virtualAddress);
    auto addr = Memory::currentVMM->allocatePages(program.imageSize / Memory::PageSize,
        static_cast<uint32_t>(Memory::PageTableFlags::AllowUserModeAccess));

    uint8_t* prog = reinterpret_cast<uint8_t*>(addr);*/
    auto prog = reinterpret_cast<uint8_t*>(map(program.virtualAddress, 
        1 + program.imageSize / Memory::PageSize,
        static_cast<uint32_t>(Memory::PageTableFlags::AllowUserModeAccess)
            //| static_cast<uint32_t>(Memory::PageTableFlags::Present)
            | static_cast<uint32_t>(Memory::PageTableFlags::AllowWrite)));

        /*map(program.virtualAddress + 0x1000, 
        1,
        static_cast<uint32_t>(Memory::PageTableFlags::AllowUserModeAccess)
            //| static_cast<uint32_t>(Memory::PageTableFlags::Present)
            | static_cast<uint32_t>(Memory::PageTableFlags::AllowWrite));*/
    //*prog = 0;
    //*(prog + 0x1000) = 0;
    //fseek(file, header.programHeaderStart + header.programHeaderEntrySize * header.programHeaderEntryCount,
    //    SEEK_SET);
    fread(prog + 148, program.imageSize, 1, file);

    //Memory::currentVMM->HACK_setNextAddress(savedAddress);

    return header.entryPoint;// - (sizeof(header) + sizeof(program) * 3); //0xa0;
    #endif
}