#pragma once

#include <stdint.h>

namespace CPU {

    bool parseACPITables();

    struct RootSystemDescriptionPointer {
        uint8_t signature[8];
        uint8_t checksum {1};
        uint8_t oemid[6];
        uint8_t revision;
        uint32_t rsdtAddress;
    } __attribute__((packed));

    RootSystemDescriptionPointer findRSDP();
    bool verifyRSDPChecksum(const RootSystemDescriptionPointer& rsdp);

    struct SystemDescriptionTableHeader {
        uint8_t signature[4];
        uint32_t length;
        uint8_t revision;
        uint8_t checksum;
        uint8_t oemid[6];
        uint8_t oemTableId[8];
        uint32_t oemRevision;
        uint32_t creatorId;
        uint32_t creatorRevision;
    } __attribute__((packed));

    struct RootSystemDescriptorTable {
        SystemDescriptionTableHeader header;
        uint32_t firstTableAddress;
    };

    RootSystemDescriptorTable* getRootSystemHeader(uintptr_t address);
    bool verifySystemHeaderChecksum(SystemDescriptionTableHeader* p);
    
    SystemDescriptionTableHeader* getAPICHeader(RootSystemDescriptorTable* rootSystemHeader, uint32_t entryCount);
}