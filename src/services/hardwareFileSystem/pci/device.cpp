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
#include "device.h"
#include <string.h>
#include <system_calls.h>
#include <services/virtualFileSystem/virtualFileSystem.h>
#include <algorithm>
#include <stdlib.h>

using namespace VirtualFileSystem;
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