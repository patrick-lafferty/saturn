#pragma once 
#include <stdint.h>

namespace APIC {
    void loadAPICStructures(uintptr_t address, uint32_t byteLength);

    enum class DescriptionFlags {
        PCAT_COMPAT = 0x1
    };

    struct LocalAPICHeader {
        uint8_t type;
        uint8_t length;
        uint8_t acpiProcessorId;
        uint8_t apicId;
        uint32_t flags;
    } __attribute__((packed));

    enum class LocalApicFlags {
        Enabled = 0x1
    };

    enum class APICType {
        Local = 0x0,
        IO = 0x1,
        Reserved
    };

    struct IOAPICHeader {
        uint8_t type;
        uint8_t length;
        uint8_t apicId;
        uint8_t reserved;
        uint32_t address;
        uint32_t systemVectorBase;
    } __attribute__((packed));
}