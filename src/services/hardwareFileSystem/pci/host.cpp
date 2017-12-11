#include "host.h"
#include <string.h>
#include <system_calls.h>
#include <services/virtualFileSystem/virtualFileSystem.h>
#include <algorithm>
#include <string.h>
#include <stdlib.h>

using namespace VirtualFileSystem;
using namespace Vostok;

namespace HardwareFileSystem::PCI {

    HostObject::HostObject() {

    }

    void HostObject::readSelf(uint32_t requesterTaskId, uint32_t requestId) {
        ReadResult result;
        result.success = true;
        result.requestId = requestId;
        ArgBuffer args {result.buffer, sizeof(result.buffer)};
        args.writeType(ArgTypes::Property);

        for (auto i = 0; i < 32; i++) {
            if (devices[i].exists()) {
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
    int HostObject::getProperty(std::string_view) {
        return -1;
    }

    void HostObject::readProperty(uint32_t requesterTaskId, uint32_t requestId, uint32_t) {
        ReadResult result;
        result.success = true;
        result.requestId = requestId;
        ArgBuffer args {result.buffer, sizeof(result.buffer)};
        args.writeType(ArgTypes::Property);

        args.writeType(ArgTypes::EndArg);

        result.recipientId = requesterTaskId;
        send(IPC::RecipientType::TaskId, &result);
    }

    void HostObject::writeProperty(uint32_t requesterTaskId, uint32_t requestId, uint32_t, ArgBuffer&) {
        replyWriteSucceeded(requesterTaskId, requestId, false);
    }

    ::Object* HostObject::getNestedObject(std::string_view name) {
        char deviceId[11];
        memset(deviceId, '\0', sizeof(deviceId));
        name.copy(deviceId, name.length());
        char* end;
        uint32_t id = strtol(deviceId, &end, 10);

        if (end != deviceId) {
            return &devices[id];
        }

        return nullptr;
    }

    /*
    HostObject specific implementation
    */

    const char* HostObject::getName() {
        return "host";
    }
}