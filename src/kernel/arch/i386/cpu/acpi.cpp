#include "acpi.h"
#include <string.h>
#include <stdio.h>

namespace CPU {
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

    bool verifyChecksum(const RootSystemDescriptionPointer& rsdp) {

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
}
