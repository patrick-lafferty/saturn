#include "apic.h"
#include <stdio.h>
#include <string.h>

namespace APIC {

    void loadAPICStructures(uintptr_t address, uint32_t byteLength) {
        uint8_t* ptr = static_cast<uint8_t*>(reinterpret_cast<void*>(address));
        uint32_t localAPICAddress;
        memcpy(&localAPICAddress, ptr, 4);
        ptr += 4;

        printf("[APIC] Local APIC Address: %x\n", localAPICAddress);

        uint32_t apicFlags;
        memcpy(&apicFlags, ptr, 4);
        ptr += 4;

        printf("[APIC] Flags: %x\n", apicFlags);

        byteLength -= 8;

        if (apicFlags & static_cast<uint32_t>(DescriptionFlags::PCAT_COMPAT)) {
            printf("[APIC] Dual 8259 detected, will be disabled\n");
        }

        printf("[APIC] Bytes remaining: %d, sizes: local %d io %d\n", byteLength, sizeof(LocalAPICHeader), sizeof(IOAPICHeader));

        while(byteLength > 0) {
            uint8_t type = *ptr;

            if (type == static_cast<uint8_t>(APICType::Local)) {
                printf("[APIC] Loading Local APIC structure\n");
                byteLength -= sizeof(LocalAPICHeader);
                ptr += sizeof(LocalAPICHeader);
            }
            else if (type == static_cast<uint8_t>(APICType::IO)) {
                printf("[APIC] Loading IO APIC structure\n");
                byteLength -= sizeof(IOAPICHeader);
                ptr += sizeof(IOAPICHeader);
            }
            else {
                uint8_t length = *(ptr + 1);
                printf("[APIC] Skipping reserved APIC structure type length %d\n", length);
                byteLength -= length;
                ptr += length   ;
            }

        }

    }
}