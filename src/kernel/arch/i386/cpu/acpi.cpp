#include "acpi.h"
#include <string.h>
#include <stdio.h>
#include <cpu/apic.h>

namespace CPU {

    void printHeader(RootSystemDescriptionPointer header) {
        kprintf("Signature:  %d %d %d %d %d %d %d %d\n", 
            header.signature[0], header.signature[1], header.signature[2], 
            header.signature[3], header.signature[4], header.signature[5],
            header.signature[6], header.signature[7]);
        kprintf("Checksum: %d\n", header.checksum);
        kprintf("Oemid: %d %d %d %d %d %d\n", 
            header.oemid[0], header.oemid[1], header.oemid[2], 
            header.oemid[3], header.oemid[4], header.oemid[5]);
        kprintf("Revision: %d\n", header.revision);
        kprintf("CreatorRevision: %d\n", header.rsdtAddress);
    }

    void printHeader(SystemDescriptionTableHeader header) {
        kprintf("Signature: %d %d %d %d\n", header.signature[0], header.signature[1], header.signature[2], header.signature[3]);
        kprintf("Length: %d\n", header.length);
        kprintf("Revision: %d\n", header.revision);
        kprintf("Checksum: %d\n", header.checksum);
        kprintf("Oemid: %d %d %d %d %d %d\n", 
            header.oemid[0], header.oemid[1], header.oemid[2], 
            header.oemid[3], header.oemid[4], header.oemid[5]);
        kprintf("OemTableId:  %d %d %d %d %d %d %d %d\n", 
            header.oemTableId[0], header.oemTableId[1], header.oemTableId[2], 
            header.oemTableId[3], header.oemTableId[4], header.oemTableId[5],
            header.oemTableId[6], header.oemTableId[7]);
        kprintf("CreatorId: %d\n", header.creatorId);
        kprintf("CreatorRevision: %d\n", header.creatorRevision);
    }

    bool parseACPITables() {
        auto rsdp = findRSDP();

        if (!verifyRSDPChecksum(rsdp)) {
            kprintf("\n[ACPI] RSDP Checksum invalid\n");
            return false;
        }

        auto rootSystemHeader = getRootSystemHeader(rsdp.rsdtAddress);

        if (verifySystemHeaderChecksum(&rootSystemHeader->header)) {
            auto apicHeader = getAPICHeader(rootSystemHeader, (rootSystemHeader->header.length - sizeof(SystemDescriptionTableHeader)) / 4);

            if (verifySystemHeaderChecksum(apicHeader)) {
                auto apicStartingAddress = reinterpret_cast<uintptr_t>(apicHeader);
                apicStartingAddress += sizeof(SystemDescriptionTableHeader);
                
                APIC::initialize();
                APIC::loadAPICStructures(apicStartingAddress, apicHeader->length - sizeof(SystemDescriptionTableHeader));
            }
            else {
                kprintf("\n[ACPI] APIC Header Checksum invalid\n");
                return false;
            }
        }
        else {
            kprintf("\n[ACPI] Root Checksum invalid\n");
            return false;
        }

        return true;
    }

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
        auto ptr = static_cast<const unsigned char*>(reinterpret_cast<const void*>(&rsdp));

        auto l = sizeof(RootSystemDescriptionPointer);
        uint8_t c = 0;
        for (auto i = 0u; i < l; i++) {
            c += (char)ptr[i];
        }

        return c == 0;
    }

    /*
    System Description Table
    */
    RootSystemDescriptorTable* getRootSystemHeader(uintptr_t address) {
        auto ptr = reinterpret_cast<void*>(address);
        
        return static_cast<RootSystemDescriptorTable*>(ptr);
    }

    bool verifySystemHeaderChecksum(SystemDescriptionTableHeader* p) {

        auto ptr = static_cast<unsigned char*>(static_cast<void*>(p));

        auto l = p->length;
        uint8_t c = 0;
        for (auto i = 0u; i < l; i++) {
            c += (char)ptr[i];
        }

        return c == 0;
    }

    SystemDescriptionTableHeader* getAPICHeader(RootSystemDescriptorTable* rootSystemHeader, uint32_t entryCount) {
        auto ptr = reinterpret_cast<uintptr_t*>(&rootSystemHeader->firstTableAddress);

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

        return nullptr;
    }
}
