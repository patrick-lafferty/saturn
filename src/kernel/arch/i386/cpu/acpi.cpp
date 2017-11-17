#include "acpi.h"
#include <string.h>
#include <stdio.h>

namespace CPU {
    /*
    Root System Description Pointer
    */
    RootSystemDescriptionPointer findRSDP() {
        auto start = reinterpret_cast<unsigned char*>(0xd00e0000);
        auto end = reinterpret_cast<unsigned char*>(0xd00fffff);

        for (auto ptr = start; ptr < end; ptr += 16) {
            if (memcmp(ptr, "RSD PTR ", 8) == 0) {
                return *reinterpret_cast<RootSystemDescriptionPointer*>(ptr);
            }
        }

        return RootSystemDescriptionPointer {};
    }

    bool verifyRSDPChecksum(const RootSystemDescriptionPointer& rsdp) {

        uint32_t check = rsdp.checksum;

        for(int i = 0; i < 8; i++) {
            check += rsdp.signature[i];
        }

        for(int i = 0; i < 6; i++) {
            check += rsdp.oemid[i];
        }

        check += rsdp.revision;
        check += (rsdp.rsdtAddress & 0xFFFF);
        check += ((rsdp.rsdtAddress >> 8) & 0xFFFF);
        check += ((rsdp.rsdtAddress >> 16) & 0xFFFF);
        check += ((rsdp.rsdtAddress >> 24) & 0xFFFF);

        return (check & 0xFF) == 0;
    }

    /*
    System Description Table
    */
    SystemDescriptionTableHeader* getRootSystemHeader(uintptr_t address) {
        auto ptr = reinterpret_cast<void*>(address);
        
        return static_cast<SystemDescriptionTableHeader*>(ptr);
    }

    bool verifySystemHeaderChecksum(SystemDescriptionTableHeader* p) {

        auto ptr = static_cast<unsigned char*>(static_cast<void*>(p));

        auto l = p->length;
        uint8_t c = 0;
        for (auto i = 0u; i < l; i++) {
            c += ptr[i];
        }

        return c == 0;
    }

    SystemDescriptionTableHeader* getAPICHeader(SystemDescriptionTableHeader* rootSystemHeader, uint32_t entryCount) {
        auto ptr = reinterpret_cast<uintptr_t*>(rootSystemHeader);
        ptr += sizeof(SystemDescriptionTableHeader) / 4;

        for (auto i = 0u; i < entryCount; i++) {
            auto address = *(ptr + i);
            auto header = static_cast<SystemDescriptionTableHeader*>(reinterpret_cast<void*>(address));

            if (memcmp(header, "APIC", 4) == 0) {
                return header;
            }
            else {
                char s[5];
                memcpy(s, header->signature, 4);
                s[4] = '\0';
            }
        }

        return rootSystemHeader;
    }
}
