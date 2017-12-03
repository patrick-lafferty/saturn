#include "pci.h"
#include <stdio.h>
#include <services.h>
#include <system_calls.h>
#include <services/hardwareFileSystem/object.h>

namespace Discovery::PCI {

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
    
    using namespace StandardConfiguration;

    HostBridge findHostBridge() {
        HostBridge bridge {};

        for (uint8_t bus = 0; bus < 255; bus++) {
            auto id = Identification{readRegister(getAddress(bus, 0, 0, 0))};

            if (id.isValid()) {
                auto classInfo = Class{readRegister(getAddress(bus, 0, 0, 0x08))};
                auto header = Header{readRegister(getAddress(bus, 0, 0, 0x0C))};

                if (classInfo.classCode == static_cast<uint8_t>(ClassCode::BridgeDevice)
                    && classInfo.subclass == static_cast<uint8_t>(BridgeDevice::HostBridge)) {
                    bridge.bus = bus;
                        
                    if (header.headerType == static_cast<uint8_t>(HeaderType::PCItoPCIBridge)) {
                        auto busInfo = PCIBridgeConfiguration::BusInfo{readRegister(getAddress(bus, 0, 0, 0x18))};
                        bridge.secondaryBus = busInfo.secondaryBusNumber;
                    }

                    bridge.isMultiFunction = header.isMultiFunction;
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
            auto id = Identification{readRegister(getAddress(bus, deviceId, 0, 0))};

            if (id.isValid()) {
                char deviceName[30];
                sprintf(deviceName, "/system/hardware/pci/host0/%d", deviceId);

                char vendorIdName[30 + 11];
                sprintf(vendorIdName, "%s/vendorId", deviceName);

                HardwareFileSystem::writeTransaction(vendorIdName, id.vendorId, Vostok::ArgTypes::Uint32);

                char deviceIdName[30 + 11];
                sprintf(deviceIdName, "%s/deviceId", deviceName);
                HardwareFileSystem::writeTransaction(deviceIdName, id.deviceId, Vostok::ArgTypes::Uint32);

                for (auto offset = 0x10u; offset < 0x28; offset += 4) {
                    char barName[30 + 6];
                    sprintf(barName, "%s/bar%d", deviceName, (offset - 0x10) / 4);
                    auto bar = readRegister(getAddress(bus, deviceId, 0, offset));
                    HardwareFileSystem::writeTransaction(barName, bar, Vostok::ArgTypes::Uint32);
                }

            }
        }
    }

    bool createPCIObject() {
        auto path = "/system/hardware/pci";
        create(path);

        IPC::MaximumMessageBuffer buffer;
        receive(&buffer);

        if (buffer.messageId == VFS::CreateResult::MessageId) {
            auto result = IPC::extractMessage<VFS::CreateResult>(buffer);
            return result.success;
        }

        return false;
    }

    void discoverDevices() {
        waitForServiceRegistered(Kernel::ServiceType::VFS);

        while (!createPCIObject()) {
            sleep(10);
        }

        auto hostBridge = findHostBridge();
        
        if (!hostBridge.exists) {
            return;
        }

        discoverDevices(hostBridge.bus); 
    }
}