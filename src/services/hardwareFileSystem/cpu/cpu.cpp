#include "cpu.h"
#include <string.h>
#include <system_calls.h>
#include <services/virtualFileSystem/virtualFileSystem.h>
#include <algorithm>

using namespace VirtualFileSystem;
using namespace Vostok;

namespace HardwareFileSystem {

    CPUObject::CPUObject() {

    }

    void CPUObject::readSelf(uint32_t requesterTaskId, uint32_t requestId) {
        ReadResult result;
        result.success = true;
        result.requestId = requestId;
        ArgBuffer args {result.buffer, sizeof(result.buffer)};
        args.writeType(ArgTypes::Property);

        args.writeValueWithType("id", ArgTypes::Cstring);
        args.writeValueWithType("features", ArgTypes::Cstring);

        args.writeType(ArgTypes::EndArg);

        result.recipientId = requesterTaskId;
        send(IPC::RecipientType::TaskId, &result);
    }

    /*
    Vostok Object property support
    */
    int CPUObject::getProperty(std::string_view) {
        return -1;
    }

    void CPUObject::readProperty(uint32_t requesterTaskId, uint32_t requestId, uint32_t) {
        ReadResult result;
        result.success = true;
        result.requestId = requestId;
        ArgBuffer args {result.buffer, sizeof(result.buffer)};
        args.writeType(ArgTypes::Property);

        args.writeType(ArgTypes::EndArg);

        result.recipientId = requesterTaskId;
        send(IPC::RecipientType::TaskId, &result);
    }

    void CPUObject::writeProperty(uint32_t requesterTaskId, uint32_t requestId, uint32_t, ArgBuffer&) {
        replyWriteSucceeded(requesterTaskId, requestId, false);
    }

    Object* CPUObject::getNestedObject(std::string_view name) {
        if (name.compare("id") == 0) {
            return &id;
        }
        else if (name.compare("features") == 0) {
            return &features;
        }
        
        return nullptr;
    }

    /*
    CPUObject specific implementation
    */

    const char* CPUObject::getName() {
        return "cpu";
    }

    bool createCPUObject() {
        auto path = "/system/hardware/cpu";
        create(path);

        IPC::MaximumMessageBuffer buffer;
        receive(&buffer);

        if (buffer.messageNamespace == IPC::MessageNamespace::VFS
            && buffer.messageId == static_cast<uint32_t>(MessageId::CreateResult)) {
            auto result = IPC::extractMessage<CreateResult>(buffer);
            return result.success;
        }

        return false;
    }

    void detectCPU() {
        while (!createCPUObject()) {
            sleep(10);
        }

        uint32_t eax, ebx, ecx, edx;

        asm("cpuid"
            : "=a" (eax), "=b" (ebx), "=c" (ecx), "=d" (edx)
            : "a" (1)
        );

        detectId(eax, ebx);
        detectFeatures(ecx, edx);
    }
}