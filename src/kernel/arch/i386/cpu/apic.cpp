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
#include "apic.h"
#include <stdio.h>
#include <string.h>
#include "rtc.h"
#include "tsc.h"

//TODO: this will all have to be overhauled to handle multiple apics (per cpu)
namespace APIC {

    void writeIOAPICRegister(uintptr_t address, uint8_t select, uint32_t value) {
        uint32_t volatile* selector = reinterpret_cast<uint32_t volatile*>(address);
        uint32_t volatile* val = reinterpret_cast<uint32_t volatile*>(address + 0x10);
        *selector = select;
        *val = value;
    }

    void writeIOAPICRegister(uintptr_t address, uint8_t registerIndex, uint32_t value, uint8_t destination) {
        writeIOAPICRegister(address, 0x10 + registerIndex * 2, value);
        writeIOAPICRegister(address, 0x10 + registerIndex * 2 + 1, destination << 24);
    }

    void writeLocalAPICRegister(Registers apicRegister, uint32_t value) {
        uint32_t volatile* ptr = reinterpret_cast<uint32_t volatile*>(0xFEE00000 + static_cast<uint32_t>(apicRegister));
        *ptr = value;
    }

    uint32_t readLocalAPICRegister(Registers apicRegister) {
        uint32_t volatile* ptr = reinterpret_cast<uint32_t volatile*>(0xFEE00000 + static_cast<uint32_t>(apicRegister));
        return *ptr;
    }

    void signalEndOfInterrupt() {
        writeLocalAPICRegister(Registers::EndOfInterrupt, 0);
    }

    void setupAPICTimer() {

        writeLocalAPICRegister(Registers::LVT_Timer, combineFlags(
            52,
            LVT_TimerMode::OneShot
        ));

        writeLocalAPICRegister(Registers::DivideConfiguration, combineFlags(DivideConfiguration::By1));
        writeLocalAPICRegister(Registers::InitialCount, 0xFFFFFFFF);

        RTC::enable(0xD);
        TSC::startCalibration();
    }

    uint32_t ticksPerMilliSecond = 0;
    int tries {0};
    const int MaxTries {8};
    bool calibrateAPICTimer() {

        if (tries == MaxTries) {
            TSC::stopCalibration();
            writeLocalAPICRegister(Registers::LVT_Timer, combineFlags(LVT_Mask::DisableInterrupt));
            uint32_t ticks = 0xFFFFFFFF - readLocalAPICRegister(Registers::CurrentCount);
            RTC::disable();
            auto ticksPerSecond = ticks * 8; //RTC was using 8Hz rate
            ticksPerMilliSecond = ticksPerSecond / (1000 * (tries + 1));
            
            writeLocalAPICRegister(Registers::InitialCount, 0x0);
            writeLocalAPICRegister(Registers::DivideConfiguration, combineFlags(DivideConfiguration::By1));

            return true;
        }
        else {
            RTC::reset();
            tries++;
            return false;
        }
    }

    void setAPICTimer(LVT_TimerMode mode, uint32_t timeInMilliseconds) {

        writeLocalAPICRegister(Registers::LVT_Timer, combineFlags(
            52,
            mode
        )); 

        writeLocalAPICRegister(Registers::InitialCount, timeInMilliseconds * ticksPerMilliSecond);
    }

    void initialize() {
        /*
        The following values/descriptions are from Intel 64 and IA-32 
        Architectures Software Developer Manual, Volume 3: System Programming Guide

        local apic state after power up
        
        the following registers are reset to all 0s:
            -Interrupt Request Register (IRR) [Read Only], In-Service Register (ISR) [Read Only], TMR, 
                Interrupt Command Register (ICR), Logical Destination Register (LDR), 
                Task Priority Register (TPR)
            -Timer initial count and current count registers
            -Divide configuration register

        -Destination Format Register (DFR) is set to all 1s
        -LVT is reset to 0s except for mask bits which are 1s
        */

        /*
        Local Vector Table Timer, offset 0x320

        Bits 0-7: Interrupt Vector Number
        Bits 8-11: Reserved
        Bit 12: Delivery Status [Read Only]
        Bits 13-15: Reserved
        Bit 16: Mask
        Bits 17-18: Timer Mode (00: One-shot, 01: Periodic, 10: TSC-Deadline)
        Bits 19-31: Reserved

        Value after reset: 0x10000
        */
        writeLocalAPICRegister(Registers::LVT_Timer, combineFlags(LVT_Mask::DisableInterrupt));


        /*
        LINT0 (offset 0x350) and LINT1 (offset 0x360)
        Bits 0-7: Interrupt Vector Number
        Bits 8-10: Delivery Mode(000: Fixed, 010: SMI, 100: NMI, 111: ExtINT, 101: INIT)
        Bit 11: Reserved
        Bit 12: Delivery Status [Read Only]
        Bit 13: Interrupt Input Pin Polarity
        Bit 14: Remote IRR [Read Only]
        Bit 15: Trigger Mode (0: Edge, 1: Level)
        Bit 16: Mask (0: enables reception of the interrupt, 1 inhibits reception)
        Bits 17-31: Reserved

        LINT1:
        -Trigger mode should always be 0 (edge sensitive), level-sensitive interrupts are not supported for LINT1

        Delivery [ExtInt] forces Trigger [level]
        */
        writeLocalAPICRegister(Registers::LINT0, combineFlags(
            LVT_DeliveryMode::ExtINT,
            LVT_Trigger::Level
        ));
        writeLocalAPICRegister(Registers::LINT1, combineFlags(
            LVT_DeliveryMode::NMI
        ));

        /*
        LVT Error Register, offset 0x370
        Bits 0-7: Vector
        Bits 8-11: Reserved
        Bit 12: Delivery Status [Read Only]
        Bits 13-15: Reserved
        Bit 16: Mask
        Bits 17-31: Reserved

        Value after reset: 0x10000
        */
        writeLocalAPICRegister(Registers::LVT_Error, combineFlags(LVT_Mask::DisableInterrupt));

        /*
        Divide Configuration Register, offset 0x3E0
        Bits 3, 1 and 0:
            000: Divide by 2
            001: Divide by 4
            010: Divide by 8
            011: Divide by 16
            100: Divide by 32
            101: Divide by 64
            110: Divide by 128
            111: Divide by 1

        Bit 2: 0
        Bits 4-31: Reserved

        Value after reset: 0
        */
        writeLocalAPICRegister(Registers::DivideConfiguration, combineFlags(DivideConfiguration::By2));

        /*
        Initial Count, offset 0x380
        Bits 0-31: Initial Count

        Value after reset: 0
        */
        writeLocalAPICRegister(Registers::InitialCount, 0x0);

        /*
        Current Count, offset 0x390
        Bits 0-31: Current Count

        Value after reset: 0
        */
        writeLocalAPICRegister(Registers::CurrentCount, 0x0);

        /*
        Interrupt Command Register, offset 0x300
        Bits 0-7: Interrupt Vector Number
        Bits 8-10: Delivery Mode
            (000: Fixed, 001: Lowest Priority, 010: SMI, 011: Reserved, 
                100: NMI, 101: INIT, 110: Start Up, 111: Reserved)
        Bit 11: Destination Mode (0: Physical, 1: Logical)
        Bit 12: Delivery Status [Read Only]
        Bit 13: Reserved
        Bit 14: Level (0: De-assert, 1: assert)
        Bit 15: Trigger Mode (0: edge, 1: level)
        Bits 16-17: Reserved
        Bits 18-19: Destination Shorthand (00: no shorthand, 01: self, 10: all including self, 11: all excluding self)
        Bits 20-31: Reserved

        offset 0x310:
        Bits 32-55: Reserved
        Bits 56-63: Destination Field

        Valid Combinations for ICR (Pentium 4 and Xeons)
        ------------------------------------------------
        No shorthand:
            Valid: Trigger [edge], Delivery [all modes], Destination [physical or logical]
            Invalid: all others

        Self:
            Valid: Trigger [edge], Delivery [fixed], Destination [ignored]
            Invalid: all others

        All including self:
            Valid: Trigger [edge], Delivery [fixed], Destination [ignored]
            Invalid: all others

        All excluding self:
            Valid: Trigger [edge], Delivery [fixed, lowest priority, nmi, init, smi, startup], Destination [ignored]
            Invalid: all others


        Valid Combinations for ICR (P6)
        -------------------------------
        No shorthand:
            Valid: Trigger [edge], Delivery [all modes], Destination [physical or logical]
                   Trigger [level], Delivery [fixed, lowest priority, nmi], Destination [physical or logical]
                   Trigger [level], Delivery [init], Destination [physical or logical]

        Self:
            Valid: Trigger [Edge], Delivery [fixed], Destination [ignored]
                   Trigger [Level], Delivery [fixed], Destination [ignored]
            
        All including self:
            Valid: Trigger [edge], Delivery [fixed], Destination [ignored]
                   Trigger [level], Delivery [fixed], Destination [ignored]

        All excluding self:
            Valid: Trigger [edge], Delivery [all modes], Destination [ignored]
                   Trigger [level], Delivery [fixed, lowest priority, nmi], Destination [ignored]
                   Trigger [level], Delivery [init], Destination [ignored]

        Value after reset: 0
        */
        writeLocalAPICRegister(Registers::InterruptCommandLower, 0x0);
        writeLocalAPICRegister(Registers::InterruptCommandHigher, 0x0);

        /*
        Logical Destination Register, offset 0xD0
        Bits 0-23: Reserved
        Bits 24-31: Logical APIC ID

        Value after reset: 0
        */
        writeLocalAPICRegister(Registers::LogicalDestination, 0x0);

        /*
        Destination Format Register, offset 0xE0
        Bits 0-27: Reserved (All 1s)
        Bits 28-31: Model (Flat model: 1111B, Cluster model: 0000B)

        Value after reset: 0xFFFFFFFF
        */
        writeLocalAPICRegister(Registers::DestinationFormat, 0xFFFFFFFF);

        /*
        Task priority register, offset 0x80
        Bits 0-3: Task Priority sub-class
        Bits 4-7: Task Priority class
        Bits 8-31: Reserved

        Value after reset: 0
        */
        writeLocalAPICRegister(Registers::TaskPriority, 0);

        /*
        Spurious Interrupt Vector Register (SVR), offset 0xF0
        Bits 0-7: Spurious Vector
        Bit 8: APIC Software Enable (0: disabled, 1: enabled)
        Bit 9: Focus Processor Checking (0: enabled, 1: disabled)
        Bits 10-11: Reserved
        Bit 12: End Of Interrupt (EOI) Broadcast Suppression (0: disabled, 1: enabled)
        Bits 13-31: Reserved

        Value after reset: 0xFF
        */
        writeLocalAPICRegister(Registers::SpuriousInterruptVector, 0x1CF);
    }

    bool VerboseAPIC {false};

    void loadAPICStructures(uintptr_t address, uint32_t byteLength) {

        uint8_t* ptr = static_cast<uint8_t*>(reinterpret_cast<void*>(address));
        uint32_t localAPICAddress;
        memcpy(&localAPICAddress, ptr, 4);
        ptr += 4;

        if (VerboseAPIC) {
            kprintf("[APIC] Local APIC Address: %x\n", localAPICAddress);
        }

        uint32_t apicFlags;
        memcpy(&apicFlags, ptr, 4);
        ptr += 4;

        byteLength -= 8;

        if (apicFlags & static_cast<uint32_t>(DescriptionFlags::PCAT_COMPAT)
            && VerboseAPIC) {
            kprintf("[APIC] Dual 8259 detected, will be disabled\n");
        }

        uintptr_t ioAPICAddress {0};

        while(byteLength > 0) {
            uint8_t type = *ptr;

            switch(static_cast<APICType>(type)) {
                case APICType::Local: {
                    auto localAPIC = reinterpret_cast<LocalAPICHeader*>(ptr);
                    
                    if (VerboseAPIC) {
                        kprintf("[APIC] Loading Local APIC structure\n");
                        kprintf("[APIC] Local APIC Id: %d, address: %d\n", localAPIC->apicId);
                    }

                    byteLength -= sizeof(LocalAPICHeader);
                    ptr += sizeof(LocalAPICHeader);
                    break;
                }

                case APICType::IO: {
                    auto ioAPIC = reinterpret_cast<IOAPICHeader*>(ptr);

                    if (VerboseAPIC) {
                        kprintf("[APIC] Loading IO APIC structure\n");
                        kprintf("[APIC] IOAPIC id: %d, address: %x, sysVecBase: %d\n", 
                        ioAPIC->apicId, ioAPIC->address, ioAPIC->systemVectorBase);
                    }

                    byteLength -= sizeof(IOAPICHeader);
                    ptr += sizeof(IOAPICHeader);
                   
                    ioAPICAddress = ioAPIC->address;

                    break;
                }

                case APICType::InterruptSourceOverride: {
                    auto interruptOverride = reinterpret_cast<InterruptSourceOverride*>(ptr);

                    if (VerboseAPIC) {
                        kprintf("[APIC] Loading InterruptSourceOverride structure\n");
                        kprintf("[APIC] NMI bus: %d, source: %d, globSysIntV: %d, flags: %d\n",
                            interruptOverride->bus, interruptOverride->source,
                            interruptOverride->globalSystemInterruptVector,
                            interruptOverride->flags);
                    }

                    byteLength -= sizeof(InterruptSourceOverride);
                    ptr += sizeof(InterruptSourceOverride);
                    
                    /*writeIOAPICRegister(ioAPICAddress, interruptOverride->globalSystemInterruptVector, 
                        combineFlags(
                            48 + interruptOverride->globalSystemInterruptVector,
                            IO_DeliveryMode::Fixed,
                            IO_DestinationMode::Physical,
                            interruptOverride->flags & 0b01 ? IO_Polarity::ActiveHigh : IO_Polarity::ActiveLow,
                            interruptOverride->flags & 0b0100 ? IO_TriggerMode::Edge : IO_TriggerMode::Level
                    ), 0);*/

                    break;
                }

                case APICType::NMI: {
                    if (VerboseAPIC) {
                        kprintf("[APIC] Loading NMI structure\n");
                    }

                    byteLength -= sizeof(LocalAPICNMI);
                    ptr += sizeof(LocalAPICNMI);
                    break;
                }

                default: {
                    uint8_t length = *(ptr + 1);

                    if (VerboseAPIC) {
                        kprintf("[APIC] Skipping reserved APIC structure type: %d length: %d\n", type, length);
                    }
                    
                    byteLength -= length;
                    ptr += length;
                }
            }

        }

        writeIOAPICRegister(ioAPICAddress, 1, combineFlags(
                49,
                IO_DeliveryMode::Fixed,
                IO_DestinationMode::Physical,
                IO_Polarity::ActiveHigh,
                IO_TriggerMode::Edge
            ), 0);

        writeIOAPICRegister(ioAPICAddress, 12, combineFlags(
                50,
                IO_DeliveryMode::Fixed,
                IO_DestinationMode::Physical,
                IO_Polarity::ActiveHigh,
                IO_TriggerMode::Edge
            ), 0);

        writeIOAPICRegister(ioAPICAddress, 8, combineFlags(
            51,
            IO_DeliveryMode::Fixed,
            IO_DestinationMode::Physical,
            IO_Polarity::ActiveHigh,
            IO_TriggerMode::Edge
        ), 0);

        /*
        change ATA irq 14 to be 53
        */
        writeIOAPICRegister(ioAPICAddress, 14, combineFlags(
            53,
            IO_DeliveryMode::Fixed,
            IO_DestinationMode::Physical,
            IO_Polarity::ActiveHigh,
            IO_TriggerMode::Edge
        ), 0);

        setupAPICTimer();
    }
}