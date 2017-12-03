#include "pci.h"
#include <stdint.h>
#include <stdio.h>
#include <services.h>
#include <system_calls.h>

namespace Discovery {

    uint32_t getAddress(uint8_t bus, uint8_t device,
        uint8_t function, uint8_t registerIndex) {

        return 0x80000000
            | (bus << 16)
            | (device << 11)
            | (function << 8)
            | registerIndex;
    }

    uint32_t readRegister(uint32_t address) {
        uint16_t configAddress {0xCF8};

        asm("outl %0, %1"
            : //no output
            : "a" (address), "d" (configAddress));

        uint32_t result {0};
        uint16_t configData {0xCFC};

        asm("inl %1, %0"
            : "=a" (result)
            : "Nd" (configData));

        return result;
    }

    struct Register00 {
        uint16_t vendorId;
        uint16_t deviceId;

        Register00(uint32_t word) {
            vendorId = word & 0xFFFF;
            deviceId = (word >> 16) & 0xFFFF;
        }

        bool exists() {
            return vendorId != 0xFFFF && deviceId != 0xFFFF;
        }
    };

    struct Register08 {
        uint8_t revisionId;
        uint8_t programmingInterface;
        uint8_t subclass;
        uint8_t classCode;

        Register08(uint32_t word) {
            revisionId = word & 0xFF;
            programmingInterface = (word >> 8) & 0xFF;
            subclass = (word >> 16) & 0xFF;
            classCode = (word >> 24) & 0xFF;
        }
    };

    struct Register0C {
        uint8_t cacheLineSize;
        uint8_t latencyTimer;
        uint8_t headerType : 7;
        bool isMultiFunction : 1;
        uint8_t builtInSelfTest;

        Register0C(uint32_t word) {
            cacheLineSize = word & 0xFF;
            latencyTimer = (word >> 8) & 0xFF;
            headerType = (word >> 16) & 0xFF;
            builtInSelfTest = (word >> 24) & 0xFF;
        }
    };

    namespace PCIBridgeHeader {
        struct Register18 {
            uint8_t primaryBusNumber;
            uint8_t secondaryBusNumber;
            uint8_t subordinateBusNumber;
            uint8_t secondaryLatencyTimer;

            Register18(uint32_t word) {
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

    HostBridge findHostBridge() {
        HostBridge bridge {};

        for (uint8_t bus = 0; bus < 255; bus++) {
            auto device = Register00{readRegister(getAddress(bus, 0, 0, 0))};

            if (device.exists()) {
                auto r8 = Register08{readRegister(getAddress(bus, 0, 0, 0x08))};
                auto rc = Register0C{readRegister(getAddress(bus, 0, 0, 0x0C))};

                if (r8.classCode == static_cast<uint8_t>(ClassCode::BridgeDevice)
                    && r8.subclass == static_cast<uint8_t>(BridgeDevice::HostBridge)) {
                    bridge.bus = bus;
                        
                    if (rc.headerType == static_cast<uint8_t>(HeaderType::PCItoPCIBridge)) {
                        auto r18 = PCIBridgeHeader::Register18{readRegister(getAddress(bus, 0, 0, 0x18))};
                        bridge.secondaryBus = r18.secondaryBusNumber;
                    }

                    bridge.isMultiFunction = rc.isMultiFunction;
                    bridge.exists = true;

                    return bridge;
                }
            }
        }

        return bridge;
    }

    void discoverDevices(uint8_t bus) {
        const uint8_t devicesPerBridge = 32;

        for (uint8_t deviceId = 1; deviceId < devicesPerBridge; deviceId++) {
            auto device = Register00{readRegister(getAddress(bus, deviceId, 0, 0))};

            if (device.exists()) {
                printf("vendor: %x device: %x\n", device.vendorId, device.deviceId);
            }
        }
    }

    void discoverPCIDevices() {
        waitForServiceRegistered(Kernel::ServiceType::Terminal);

        auto hostBridge = findHostBridge();
        
        if (!hostBridge.exists) {
            return;
        }

        discoverDevices(hostBridge.bus); 
    }
}