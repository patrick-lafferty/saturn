#pragma once

#include <stdint.h>

namespace CPU {

    struct RootSystemDescriptionPointer {
        uint8_t signature[8];
        uint8_t checksum {1};
        uint8_t oemid[6];
        uint8_t revision;
        uint32_t rsdtAddress;
        uint32_t length;
        uint64_t xsdtAddress;
        uint8_t extendedChecksum;
        uint8_t reserved[3];
    } __attribute__((packed));

    RootSystemDescriptionPointer findRSDP();
    bool verifyChecksum(const RootSystemDescriptionPointer& rsdp);
    //void findRSDP();  
}