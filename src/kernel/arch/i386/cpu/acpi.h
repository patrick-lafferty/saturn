#pragma once

#include <stdint.h>

namespace CPU {

    struct RootSystemDescriptionPointer {
        uint8_t signature[8];
        uint8_t checksum {1};
        uint8_t oemid[6];
        uint8_t revision;
        uint32_t rsdtAddress;
    } __attribute__((packed));

    RootSystemDescriptionPointer findRSDP();
    bool verifyRSDPChecksum(const RootSystemDescriptionPointer& rsdp);

    struct RootSystemDescriptorTable {
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

    SystemDescriptionTableHeader* getRootSystemHeader(uintptr_t address);
    bool verifySystemHeaderChecksum(SystemDescriptionTableHeader* p);
    
    SystemDescriptionTableHeader* getAPICHeader(SystemDescriptionTableHeader* rootSystemHeader, uint32_t entryCount);
}