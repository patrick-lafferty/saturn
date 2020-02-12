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
#include "acpi.h"
#include <string.h>
#include <stdio.h>
#include <cpu/apic.h>

namespace CPU {

    bool verifyChecksum(uint8_t* table, size_t length) {
        uint8_t c = 0;

        for (auto i = 0u; i < length; i++) {
            c += (char)table[i];
        }

        return c == 0;
    }

    SystemDescriptionTableHeader* getAPICHeader(RootSystemDescriptorTable* rootSystemHeader, uint32_t entryCount) {
        auto ptr = reinterpret_cast<uint32_t*>(&rootSystemHeader->firstTableAddress);

        for (auto i = 0u; i < entryCount; i++) {
            auto address = *(ptr + i);
            auto header = reinterpret_cast<SystemDescriptionTableHeader*>(address);

            if (memcmp(header, "APIC", 4) == 0) {
                return header;
            }
            else {
                char s[5];
                memcpy(s, header->signature, 4);
                s[4] = '\0';
            }
        }

        return nullptr;
    }

    std::optional<ACPITableHeader> parseACPITables(uintptr_t rdspAddress) {
        
        auto rsdp = reinterpret_cast<RootSystemDescriptionPointer*>(rdspAddress);

        if (!verifyChecksum(reinterpret_cast<uint8_t*>(rsdp), sizeof(RootSystemDescriptionPointer))) {
            kprintf("\n[ACPI] RSDP Checksum invalid\n");
            return {};
        }

        auto rootSystemHeader = reinterpret_cast<RootSystemDescriptorTable*>(rsdp->rsdtAddress);

        if (verifyChecksum(reinterpret_cast<uint8_t*>(&rootSystemHeader->header), rootSystemHeader->header.length)) {
            auto apicHeader = getAPICHeader(rootSystemHeader, (rootSystemHeader->header.length - sizeof(SystemDescriptionTableHeader)) / 4);

            if (verifyChecksum(reinterpret_cast<uint8_t*>(apicHeader), apicHeader->length)) {
                auto apicStartingAddress = reinterpret_cast<uintptr_t>(apicHeader);
                apicStartingAddress += sizeof(SystemDescriptionTableHeader);

                return ACPITableHeader {apicStartingAddress, apicHeader->length - sizeof(SystemDescriptionTableHeader)};                
            }
            else {
                kprintf("\n[ACPI] APIC Header Checksum invalid\n");
                return {};
            }
        }
        else {
            kprintf("\n[ACPI] Root Checksum invalid\n");
            return {};
        }
    }
}
