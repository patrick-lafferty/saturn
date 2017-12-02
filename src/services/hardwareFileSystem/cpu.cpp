#include "cpu.h"
#include <string.h>
#include <system_calls.h>
#include <services/virtualFileSystem/virtualFileSystem.h>

using namespace VFS;
using namespace Vostok;

namespace HardwareFileSystem {

     void replyWriteSucceeded(uint32_t requesterTaskId, uint32_t requestId, bool success) {
        WriteResult result;
        result.requestId = requestId;
        result.success = success;
        result.recipientId = requesterTaskId;
        send(IPC::RecipientType::TaskId, &result);
    }

    CPUObject::CPUObject() {

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
        if (name.compare("test") == 0) {
            return static_cast<int>(PropertyId::test);
        }

        return -1;
    }

    void CPUObject::readProperty(uint32_t requesterTaskId, uint32_t requestId, uint32_t propertyId) {
        ReadResult result;
        result.success = true;
        result.requestId = requestId;
        ArgBuffer args {result.buffer, sizeof(result.buffer)};
        args.writeType(ArgTypes::Property);

        switch(static_cast<PropertyId>(propertyId)) {
            case PropertyId::test: {
                args.writeValueWithType(test, ArgTypes::Cstring);
                break;
            }
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
            case PropertyId::test: {
                auto x = args.read<char*>(ArgTypes::Cstring);

                if (!args.hasErrors()) {
                    memcpy(test, x, sizeof(test));
                    replyWriteSucceeded(requesterTaskId, requestId, true);
                    return;
                }
            }
        }

        replyWriteSucceeded(requesterTaskId, requestId, false);
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

        auto openResult = openSynchronous("/system/hardware/cpu/test");

        if (openResult.success) {
            auto readResult = readSynchronous(openResult.fileDescriptor, 0);

            if (readResult.success) {
                ArgBuffer args{readResult.buffer, sizeof(readResult.buffer)};

                auto type = args.readType();

                if (type == ArgTypes::Property) {
                    args.write("cpu detected", ArgTypes::Cstring);
                }

                write(openResult.fileDescriptor, readResult.buffer, sizeof(readResult.buffer));
                receiveAndIgnore();
                close(openResult.fileDescriptor);
                receiveAndIgnore();
            }
        }
    }
}