#include "technology.h"
#include <string.h>
#include <system_calls.h>
#include <services/virtualFileSystem/virtualFileSystem.h>
#include <algorithm>

using namespace VFS;
using namespace Vostok;

namespace HardwareFileSystem {

    CPUTechnologyObject::CPUTechnologyObject() {
        technology = 0;        
    }

    void CPUTechnologyObject::readSelf(uint32_t requesterTaskId, uint32_t requestId) {
        ReadResult result;
        result.success = true;
        result.requestId = requestId;
        ArgBuffer args {result.buffer, sizeof(result.buffer)};
        args.writeType(ArgTypes::Property);

        args.writeValueWithType("eist", ArgTypes::Cstring);
        args.writeValueWithType("tm2", ArgTypes::Cstring);

        args.writeType(ArgTypes::EndArg);

        result.recipientId = requesterTaskId;
        send(IPC::RecipientType::TaskId, &result);
    }

    /*
    Vostok Object property support
    */
    int CPUTechnologyObject::getProperty(std::string_view name) {
        if (name.compare("eist") == 0) {
            return static_cast<int>(PropertyId::EIST);
        }
        else if (name.compare("tm2") == 0) {
            return static_cast<int>(PropertyId::TM2);
        }

        return -1;
    }

    void CPUTechnologyObject::readProperty(uint32_t requesterTaskId, uint32_t requestId, uint32_t propertyId) {
        ReadResult result;
        result.success = true;
        result.requestId = requestId;
        ArgBuffer args {result.buffer, sizeof(result.buffer)};
        args.writeType(ArgTypes::Property);

        args.writeValueWithType((technology & (1 << propertyId)) > 0, ArgTypes::Uint32);

        args.writeType(ArgTypes::EndArg);

        result.recipientId = requesterTaskId;
        send(IPC::RecipientType::TaskId, &result);
    }

    void CPUTechnologyObject::writeProperty(uint32_t requesterTaskId, uint32_t requestId, uint32_t propertyId, ArgBuffer& args) {
        auto type = args.readType();

        if (type != ArgTypes::Property) {
            replyWriteSucceeded(requesterTaskId, requestId, false);
            return;
        }

        auto x = args.read<uint32_t>(ArgTypes::Uint32);

        if (!args.hasErrors()) {
            if (x > 0) {
                technology |= (1 << propertyId);
            }
            replyWriteSucceeded(requesterTaskId, requestId, true);
            return;
        }

        replyWriteSucceeded(requesterTaskId, requestId, false);
    }

    /*
    CPU Features Object specific implementation
    */

    const char* CPUTechnologyObject::getName() {
        return "technology";
    }

    void detectTechnology(uint32_t ecx) {

        auto eist = (ecx & (1u << 7));
        auto tm2 = (ecx & (1u << 8));

        writeTransaction("/system/hardware/cpu/features/technology/eist", eist, ArgTypes::Uint32);
        writeTransaction("/system/hardware/cpu/features/extensions/tm2", tm2, ArgTypes::Uint32);
    }
}