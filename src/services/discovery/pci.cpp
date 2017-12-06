#include "pci.h"
#include <stdio.h>
#include <services.h>
#include <system_calls.h>
#include <services/hardwareFileSystem/object.h>
#include <vector>
#include <services/startup/startup.h>

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

                    bridge.isMultiFunction = header.headerType & (1 << 7);
                    bridge.exists = true;

                    return bridge;
                }
            }
        }

        return bridge;
    }

    template<typename T>
    void performHardwareTransaction(const char* path, const char* subpath, T value, Vostok::ArgTypes type) {
        char buffer[100];
        memset(buffer, '\0', sizeof(buffer));
        sprintf(buffer, path, subpath);
        HardwareFileSystem::writeTransaction(buffer, value, type);
    }

    std::vector<Device> discoverDevices(uint8_t bus) {
        std::vector<Device> devices;

        const uint8_t devicesPerBridge = 32;

        for (uint8_t deviceId = 1; deviceId < devicesPerBridge; deviceId++) {
            auto id = Identification{readRegister(getAddress(bus, deviceId, 0, 0))};

            if (!id.isValid()) {
                continue;
            }

            char deviceName[30];
            memset(deviceName, '\0', sizeof(deviceName));
            sprintf(deviceName, "/system/hardware/pci/host0/%d", deviceId);

            performHardwareTransaction("%s/vendorId", deviceName, id.vendorId, Vostok::ArgTypes::Uint32);
            performHardwareTransaction("%s/deviceId", deviceName, id.deviceId, Vostok::ArgTypes::Uint32);

            auto header = Header{readRegister(getAddress(bus, deviceId, 0, 0x0c))};
            auto lastFunction = 1;
            if (header.isMultiFunction()) {
                lastFunction = 8;
            }

            for (uint8_t functionId = 0; functionId < lastFunction; functionId++) {
                id = Identification{readRegister(getAddress(bus, deviceId, functionId, 0))};

                /*
                even though a device may be multifunctional, not all functions exist.
                it could have only functions 0 and 4 and still be valid
                */
                if (!id.isValid()) {
                    continue;
                }

                auto classReg = Class{readRegister(getAddress(bus, deviceId, functionId, 0x08))};
                char functionName[30 + 2];
                memset(functionName, '\0', sizeof(functionName));
                sprintf(functionName, "%s/%d", deviceName, functionId);

                performHardwareTransaction("%s/vendorId", functionName, id.vendorId, Vostok::ArgTypes::Uint32);
                performHardwareTransaction("%s/classCode", functionName, classReg.classCode, Vostok::ArgTypes::Uint32);
                performHardwareTransaction("%s/subclassCode", functionName, classReg.subclass, Vostok::ArgTypes::Uint32);
                
                for (auto offset = 0x10u; offset < 0x28; offset += 4) {
                    char barName[30 + 8];
                    memset(barName, '\0', sizeof(barName));
                    sprintf(barName, "%s/bar%d", functionName, (offset - 0x10) / 4);
                    auto bar = readRegister(getAddress(bus, deviceId, functionId, offset));// & 0xFFFFFFF0;
                    HardwareFileSystem::writeTransaction(barName, bar, Vostok::ArgTypes::Uint32);
                }

                devices.push_back({id, deviceId});
            }
        }

        return devices;
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

    KnownDevices getDeviceType(Identification id) {

        if (id.vendorId == 0x1234 && id.deviceId == 0x1111) {
            return KnownDevices::BochsVBE;
        }

        return KnownDevices::Unknown;
    }

    void loadDriver(KnownDevices type) {
        return;//dont want this for virtualbox yet

        switch(type) {
            case KnownDevices::BochsVBE: {
                Kernel::LinearFrameBufferFound found;
                found.address = 0xfd000008;
                send(IPC::RecipientType::ServiceRegistryMailbox, &found);
                Startup::runProgram("/bin/bochsGraphicsAdaptor.service");
                break;
            }
            default: {

            }
        }
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

        if (hostBridge.isMultiFunction) kprintf("host is MF\n");

        auto devices = discoverDevices(hostBridge.bus); 
        bool loaded[static_cast<int>(KnownDevices::Unknown)];

        for (auto& device : devices) {
            auto type = getDeviceType(device.id);

            if (type != KnownDevices::Unknown
                && !loaded[static_cast<int>(type)]) {

                loaded[static_cast<int>(type)] = true;
                loadDriver(type);
            }
        }
    }
}