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

#include "elf.h"
#include "print.h"

namespace Elf {

    bool verifyHeader(Header* header) {

        if (header->identification[0] != 0x7f
            || header->identification[1] != 'E'
            || header->identification[2] != 'L'
            || header->identification[3] != 'F') {
            printString("ElfVerify: Invalid Magic bytes", 0, 40);
            return false;
        }

        if (header->identification[static_cast<int>(IdentifierBits::Class)]
            != static_cast<int>(Class::Class64)) {
            printString("ElfVerify: Invalid class", 0, 40);
            return false;
        }

        if (header->identification[static_cast<int>(IdentifierBits::DataEncoding)]
            != static_cast<int>(DataEncoding::LittleEndian)) {
            printString("ElfVerify: Invalid data encoding", 0, 40);
            return false;
        }

        if (header->identification[static_cast<int>(IdentifierBits::Version)]
            != 1) {
            //version has the value EV_CURRENT which is defined as 1
            printString("ElfVerify: Invalid version", 0, 40);
            return false;
        }

        if (header->objectFileType != static_cast<int>(Type::Executable)) {
            printString("ElfVerify: Invalid object type", 0, 40);
            return false;
        }

        if (header->entryPoint == 0) {
            printString("ElfVerify: Invalid entry point", 0, 40);
            return false;
        }

        return true;
    }

    bool loadElfExecutable(uintptr_t address, Program& program) {
        auto header = reinterpret_cast<Header*>(address);

        if (!verifyHeader(header)) {
            return false;
        }

        auto programStart = address + header->programHeaderOffset;
        program.startAddress = address;
        program.entryPoint = header->entryPoint;
        program.headerCount = 0;

        for (auto i = 0u; i < header->programHeaderEntryCount; i++) {
            auto programHeader = reinterpret_cast<ProgramHeader*>(
                programStart + header->programHeaderEntrySize * i
            );

            switch (static_cast<ProgramType>(programHeader->type)) {
                case ProgramType::Null: {
                    continue;
                    break;
                }
                case ProgramType::Load: {
                    #if VERBOSE
                        printString("[ELF] ProgHeader Load ", 0);
                    #endif

                    if (program.headerCount >= MAX_PROGRAM_HEADERS) {
                        printString("[ELF] Found too many LOAD program headers", 0);
                        return false;
                    }

                    program.headers[program.headerCount] = programHeader;
                    program.headerCount++;

                    break;
                }
                case ProgramType::GNUStack: {
                    #if VERBOSE
                        printString("[ELF] ProgHeader GNU_STACK ", 0);
                    #endif
                    break;
                }
                default: {
                    #if VERBOSE
                        printString("header ", 0);
                        printInteger(programHeader->type, 0, 11);
                    #endif
                    break;
                }
            }
        }

        return true;
    }
}