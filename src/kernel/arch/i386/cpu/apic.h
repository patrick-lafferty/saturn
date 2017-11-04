#pragma once 
#include <stdint.h>

namespace APIC {
    void writeLocalAPICRegister(uint32_t offset, uint32_t value);
    void signalEndOfInterrupt();

    void initialize();
    void loadAPICStructures(uintptr_t address, uint32_t byteLength);

    enum class Registers {
        TaskPriority = 0x80,
        EndOfInterrupt = 0xB0,
        LogicalDestination = 0xD0,
        DestinationFormat = 0xE0,
        SpuriousInterruptVector = 0xF0,
        InterruptCommandLower = 0x300,
        InterruptCommandHigher = 0x310,
        LVT_Timer = 0x320,
        LINT0 = 0x350,
        LINT1 = 0x360,
        LVT_Error = 0x370,
        InitialCount = 0x380,
        CurrentCount= 0x390,
        DivideConfiguration = 0x3E0,
    };

    enum class LVT_DeliveryMode {
        Fixed = 0b0000'0000'0000,
        SMI = 0b0100'0000'0000,
        NMI = 0b0100'0000'0000,
        ExtINT = 0b0111'0000'0000,
        INIT = 0b0101'0000'0000
    };

    enum class LVT_Trigger {
        Edge = 0x0000,
        Level = 0x8000
    };

    enum class LVT_Mask {
        EnableInterrupt = 0x0,
        DisableInterrupt = 0x10000
    };

    template<typename... Arg> uint32_t combineFlags(Arg... args) {
        return (static_cast<uint32_t>(args) | ...);
    }

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

    enum class IO_DeliveryMode {
        Fixed = 0b000'0000'0000,
        LowestPriority = 0b001'0000'0000,
        SMI = 0b010'0000'0000,
        NMI = 0b100'0000'0000,
        INIT = 0b101'0000'0000,
        ExtINT = 0b111'0000'0000
    };

    enum class IO_DestinationMode {
        Physical = 0b0000'0000'0000,
        Logical = 0b1000'0000'0000
    };

    enum class IO_Polarity {
        ActiveHigh = 0b00'0000'0000'0000,
        ActiveLow = 0b10'0000'0000'0000
    };

    enum class IO_TriggerMode {
        Edge = 0b0000'0000'0000'0000,
        Level = 0b1000'0000'0000'0000
    };

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