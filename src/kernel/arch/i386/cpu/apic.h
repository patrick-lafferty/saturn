#pragma once 
#include <stdint.h>

namespace APIC {
    void writeLocalAPICRegister(uint32_t offset, uint32_t value);

    void initialize();
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
        Local = 0,
        IO = 1,
        InterruptSourceOverride = 2,
        NMI = 4,
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

    struct InterruptSourceOverride {
        uint8_t type;
        uint8_t length;
        uint8_t bus;
        uint8_t source;
        uint32_t globalSystemInterruptVector;
        uint16_t flags;
    } __attribute__((packed));

    struct LocalAPICNMI {
        uint8_t type;
        uint8_t length;
        uint8_t acpiProcessorId;
        uint16_t flags;
        uint8_t intiPin;
    } __attribute__((packed));
}