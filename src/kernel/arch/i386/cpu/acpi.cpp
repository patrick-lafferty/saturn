#include "acpi.h"
#include <string.h>
#include <stdio.h>

namespace CPU {
    /*
    Root System Description Pointer
    */
    RootSystemDescriptionPointer findRSDP() {
        auto start = reinterpret_cast<unsigned char*>(0x000e0000);
        auto end = reinterpret_cast<unsigned char*>(0x000fffff);

        for (auto ptr = start; ptr < end; ptr += 16) {
            if (memcmp(ptr, "RSD PTR ", 8) == 0) {
                printf("Found RDSP at: %x\n", ptr);
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
    SystemDescriptionTableHeader findSDT(uintptr_t address) {
        auto ptr = reinterpret_cast<void*>(address);
        
        return *static_cast<SystemDescriptionTableHeader*>(ptr);
    }

    bool verifySDTChecksum(const SystemDescriptionTableHeader& sdt) {
        uint32_t check = sdt.checksum;

        for(int i = 0; i < 4; i++) {
            check += sdt.signature[i];
        }

        check += sdt.length;
        check += sdt.revision;

        for (int i = 0; i < 6; i++) {
            check += sdt.oemid[i];
        }

        for (int i = 0; i < 8; i++) {
            check += sdt.oemTableId[i];
        }

        check += sdt.oemRevision;
        check += sdt.creatorId;
        check += sdt.creatorRevision;

        int entryArrayLength = (sdt.length - sizeof(SystemDescriptionTableHeader)) / 4;
        printf("Entry array length: %d\n", entryArrayLength);
        
        const void* ptr = static_cast<const void*>(&sdt);
        auto intPtr = static_cast<const int*>(ptr);
        intPtr += sizeof(SystemDescriptionTableHeader);

        for(int i = 0; i < entryArrayLength; i++) {
            check += *intPtr++;
        }

        return (check & 0xFF) == 0;
    }
}
