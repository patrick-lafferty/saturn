/*
Copyright (c) 2017, Patrick Lafferty
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

    * Redistributions of source code must retain the above copyright
      notice, this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright
      notice, this list of conditions and the following disclaimer in the
      documentation and/or other materials provided with the distribution.
    * Neither the name of the copyright holder nor the names of its 
      contributors may be used to endorse or promote products derived from 
      this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR 
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/
#pragma once 
#include <stdint.h>

namespace APIC {
    void writeLocalAPICRegister(uint32_t offset, uint32_t value);
    void signalEndOfInterrupt();

    void initialize();
    void loadAPICStructures(uintptr_t address, uint32_t byteLength);
    bool calibrateAPICTimer();


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

    enum class LVT_TimerMode {
        OneShot = 0b000'0000'0000'0000'0000,
        Periodic = 0b010'0000'0000'0000'0000,
        TSCDeadline = 0b100'0000'0000'0000'0000,
    };

    void setAPICTimer(LVT_TimerMode mode, uint32_t timeInMilliseconds);

    enum class DivideConfiguration {
        By2 = 0b000,
        By4 = 0b001,
        By8 = 0b010,
        By16 = 0b011,
        By32 = 0b100,
        By64 = 0b101,
        By128 = 0b110,
        By1 = 0b111,
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

    enum class KnownInterrupt {
        Keyboard = 49,
        Mouse = 50,
        RTC = 51,
        LocalAPICTimer = 52,
        ATA = 53
    };
}