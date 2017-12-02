#include "cpu.h"
#include <string.h>
#include <system_calls.h>
#include <services/virtualFileSystem/virtualFileSystem.h>
#include <algorithm>

using namespace VFS;
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

        args.writeType(ArgTypes::EndArg);

        result.recipientId = requesterTaskId;
        send(IPC::RecipientType::TaskId, &result);
    }

    /*
    Vostok Object function support
    */
    int CPUObject::getFunction(std::string_view name) {
        return -1;
    }

    void CPUObject::readFunction(uint32_t requesterTaskId, uint32_t requestId, uint32_t functionId) {
        describeFunction(requesterTaskId, requestId, functionId);
    }

    void CPUObject::writeFunction(uint32_t requesterTaskId, uint32_t requestId, uint32_t functionId, ArgBuffer& args) {
    }

    void CPUObject::describeFunction(uint32_t requesterTaskId, uint32_t requestId, uint32_t functionId) {
    }

    /*
    Vostok Object property support
    */
    int CPUObject::getProperty(std::string_view name) {

        return -1;
    }

    void CPUObject::readProperty(uint32_t requesterTaskId, uint32_t requestId, uint32_t propertyId) {
        ReadResult result;
        result.success = true;
        result.requestId = requestId;
        ArgBuffer args {result.buffer, sizeof(result.buffer)};
        args.writeType(ArgTypes::Property);

        switch(static_cast<PropertyId>(propertyId)) {
        }

        args.writeType(ArgTypes::EndArg);

        result.recipientId = requesterTaskId;
        send(IPC::RecipientType::TaskId, &result);
    }

    void CPUObject::writeProperty(uint32_t requesterTaskId, uint32_t requestId, uint32_t propertyId, ArgBuffer& args) {
        auto type = args.readType();

        if (type != ArgTypes::Property) {
            replyWriteSucceeded(requesterTaskId, requestId, false);
            return;
        }

        switch(static_cast<PropertyId>(propertyId)) {
        }

        replyWriteSucceeded(requesterTaskId, requestId, false);
    }

    Object* CPUObject::getNestedObject(std::string_view name) {
        if (name.compare("id") == 0) {
            return &id;
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

        if (buffer.messageId == VFS::CreateResult::MessageId) {
            auto result = IPC::extractMessage<VFS::CreateResult>(buffer);
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
    }
}