#include "device.h"
#include <string.h>
#include <system_calls.h>
#include <services/virtualFileSystem/virtualFileSystem.h>
#include <algorithm>
#include <stdlib.h>

using namespace VFS;
using namespace Vostok;

namespace HardwareFileSystem::PCI {

    DeviceObject::DeviceObject() {
        vendorId = 0xFFFF;
        deviceId = 0xFFFF;
    }

    void DeviceObject::readSelf(uint32_t requesterTaskId, uint32_t requestId) {
        ReadResult result;
        result.success = true;
        result.requestId = requestId;
        ArgBuffer args {result.buffer, sizeof(result.buffer)};
        args.writeType(ArgTypes::Property);

        args.writeValueWithType("vendorId", ArgTypes::Cstring);
        args.writeValueWithType("deviceId", ArgTypes::Cstring);

        for (auto i = 0; i < 8; i++) {
            if (function[i].exists()) {
                args.writeValueWithType(i, ArgTypes::Uint32);
            }
        }

        args.writeType(ArgTypes::EndArg);

        result.recipientId = requesterTaskId;
        send(IPC::RecipientType::TaskId, &result);
    }

    /*
    Vostok Object property support
    */
    int DeviceObject::getProperty(std::string_view name) {
        if (name.compare("vendorId") == 0) {
            return static_cast<int>(PropertyId::VendorId);
        }
        else if (name.compare("deviceId") == 0) {
            return static_cast<int>(PropertyId::DeviceId);
        }

        return -1;
    }

    void DeviceObject::readProperty(uint32_t requesterTaskId, uint32_t requestId, uint32_t propertyId) {
        ReadResult result;
        result.success = true;
        result.requestId = requestId;
        ArgBuffer args {result.buffer, sizeof(result.buffer)};
        args.writeType(ArgTypes::Property);

        switch(static_cast<PropertyId>(propertyId)) {
            case PropertyId::VendorId: {
                args.writeValueWithType(vendorId, ArgTypes::Uint32);
                break;
            }
            case PropertyId::DeviceId: {
                args.writeValueWithType(deviceId, ArgTypes::Uint32);
                break;
            }
        }

        args.writeType(ArgTypes::EndArg);

        result.recipientId = requesterTaskId;
        send(IPC::RecipientType::TaskId, &result);
    }

    void DeviceObject::writeProperty(uint32_t requesterTaskId, uint32_t requestId, uint32_t propertyId, ArgBuffer& args) {
        auto type = args.readType();

        if (type != ArgTypes::Property) {
            replyWriteSucceeded(requesterTaskId, requestId, false);
            return;
        }

        switch(static_cast<PropertyId>(propertyId)) {
            case PropertyId::VendorId: {
                auto x = args.read<uint32_t>(ArgTypes::Uint32);

                if (!args.hasErrors()) {
                    vendorId = x;
                    replyWriteSucceeded(requesterTaskId, requestId, true);
                    return;
                }

                break;
            }
            case PropertyId::DeviceId: {
                auto x = args.read<uint32_t>(ArgTypes::Uint32);

                if (!args.hasErrors()) {
                    deviceId = x;
                    replyWriteSucceeded(requesterTaskId, requestId, true);
                    return;
                }

                break;
            }
        }

        replyWriteSucceeded(requesterTaskId, requestId, false);
    }

    ::Object* DeviceObject::getNestedObject(std::string_view name) {

        char functionId[11];
        memset(functionId, '\0', sizeof(functionId));
        name.copy(functionId, name.length());
        char* end;
        uint32_t id = strtol(functionId, &end, 10);

        if (end != functionId && id < 8) {
            return &function[id];
        }

        return nullptr;
    }

    /*
    DeviceObject specific implementation
    */

    const char* DeviceObject::getName() {
        return "device";
    }

    bool DeviceObject::exists() const {
        return vendorId != 0xFFFF && deviceId != 0xFFFF;
    }

    void DeviceObject::summary(uint32_t requesterTaskId, uint32_t requestId) {
        ReadResult result;
        result.success = true;
        result.requestId = requestId;
        ArgBuffer args {result.buffer, sizeof(result.buffer)};
        args.writeType(ArgTypes::Property);

        char buffer[250];
        memset(buffer, '\0', sizeof(buffer));
        /*auto length = sprintf(buffer, "vendor: %x\n", vendorId);
        length += sprintf(buffer + length, "deviceId: %x\n", deviceId);
        length += sprintf(buffer + length, "class: %x\n", classCode);
        length += sprintf(buffer + length, "subclass: %x\n", subclassCode);
        */

        args.writeValueWithType(buffer, ArgTypes::Cstring);

        args.writeType(ArgTypes::EndArg);

        result.recipientId = requesterTaskId;
        send(IPC::RecipientType::TaskId, &result);
    }
}