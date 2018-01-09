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

namespace Discovery::PCI {

    void discoverDevices(); 

    namespace StandardConfiguration {
        /*
        This is Configuration Space register offset 0x00
        */
        struct Identification {
            uint16_t vendorId;
            uint16_t deviceId;

            Identification() {}

            Identification(uint32_t word) {
                vendorId = word & 0xFFFF;
                deviceId = (word >> 16) & 0xFFFF;
            }

            bool isValid() {
                return vendorId != 0xFFFF;// && deviceId != 0xFFFF;
            }
        };

        /*
        This is Configuration Space register offset 0x08
        */
        struct Class {
            uint8_t revisionId;
            uint8_t programmingInterface;
            uint8_t subclass;
            uint8_t classCode;

            Class() {}

            Class(uint32_t word) {
                revisionId = word & 0xFF;
                programmingInterface = (word >> 8) & 0xFF;
                subclass = (word >> 16) & 0xFF;
                classCode = (word >> 24) & 0xFF;
            }

            uint32_t getWord() {
                uint32_t result {0};

                result = revisionId
                    | (programmingInterface << 8)
                    | (subclass << 16)
                    | (classCode << 24);

                return result;
            }
        };

        /*
        This is Configuration Space register offset 0x0C
        */
        struct Header {
            uint8_t cacheLineSize;
            uint8_t latencyTimer;
            uint8_t headerType;// : 7;
            //bool isMultiFunction : 1;
            uint8_t builtInSelfTest;

            bool isMultiFunction() {
                return headerType & 0x80;
            }

            Header(uint32_t word) {
                cacheLineSize = word & 0xFF;
                latencyTimer = (word >> 8) & 0xFF;
                headerType = (word >> 16) & 0xFF;
                builtInSelfTest = (word >> 24) & 0xFF;
            }
        };

        /*
        This is Configuration Space register offset 0x3C
        */
        struct Interrupt {
            uint8_t interruptLine;
            uint8_t interruptPin;
            uint8_t minGrant;
            uint8_t maxLatency;

            Interrupt(uint32_t word) {
                interruptLine = word & 0xFF;
                interruptPin = (word >> 8) & 0xFF;
                minGrant = (word >> 16) & 0xFF;
                maxLatency = (word >> 24) & 0xFF; 
            }
        };
    }

    namespace PCIBridgeConfiguration {
        /*
        This is Configuration Space register offset 0x18
        */
        struct BusInfo {
            uint8_t primaryBusNumber;
            uint8_t secondaryBusNumber;
            uint8_t subordinateBusNumber;
            uint8_t secondaryLatencyTimer;

            BusInfo(uint32_t word) {
                primaryBusNumber = word & 0xFF;
                secondaryBusNumber = (word >> 8) & 0xFF;
                subordinateBusNumber = (word >> 16) & 0xFF;
                secondaryLatencyTimer = (word >> 24) & 0xFF;
            }
        };
    }

    enum class ClassCode {
        Legacy,
        MassStorageController,
        NetworkController,
        DisplayController,
        MultimediaController,
        MemoryController,
        BridgeDevice,
        SimpleCommunicationsController,
        BaseSystemPeripherals,
        InputDevices,
        DockingStations,
        Processors,
        SerialBusControllers,
        WirelessControllers,
        IntelligentIOControllers,
        SatelliteCommunicationsControllers,
        EncryptionDecryptionControllers,
        DataAquisitionAndSignalProcessingControllers,
        Reserved,
        Undefined = 0xFF
    };

    enum class BridgeDevice {
        HostBridge,
        PCItoPCIBridge,
    };

    struct HostBridge {
        uint8_t bus;
        uint8_t secondaryBus;
        bool isMultiFunction;
        bool exists {false};
    };

    enum class HeaderType {
        Standard,
        PCItoPCIBridge,
        CardBusBridge
    };

    struct Device {
        StandardConfiguration::Identification id;
        StandardConfiguration::Class classCode;
        uint8_t index;
        uint8_t functionId;
        uint32_t bars[6];
    };

    enum class KnownDevices {
        BochsVBE,
        ATADrive,
        Unknown
    };
}