#pragma once

#include <stdint.h>

namespace Discovery {

    void discoverPCIDevices(); 


    namespace PCI {

        namespace StandardConfiguration {
            /*
            This is Configuration Space register offset 0x00
            */
            struct Identification {
                uint16_t vendorId;
                uint16_t deviceId;

                Identification(uint32_t word) {
                    vendorId = word & 0xFFFF;
                    deviceId = (word >> 16) & 0xFFFF;
                }

                bool isValid() {
                    return vendorId != 0xFFFF && deviceId != 0xFFFF;
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

                Class(uint32_t word) {
                    revisionId = word & 0xFF;
                    programmingInterface = (word >> 8) & 0xFF;
                    subclass = (word >> 16) & 0xFF;
                    classCode = (word >> 24) & 0xFF;
                }
            };

            /*
            This is Configuration Space register offset 0x0C
            */
            struct Header {
                uint8_t cacheLineSize;
                uint8_t latencyTimer;
                uint8_t headerType : 7;
                bool isMultiFunction : 1;
                uint8_t builtInSelfTest;

                Header(uint32_t word) {
                    cacheLineSize = word & 0xFF;
                    latencyTimer = (word >> 8) & 0xFF;
                    headerType = (word >> 16) & 0xFF;
                    builtInSelfTest = (word >> 24) & 0xFF;
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

    }
}