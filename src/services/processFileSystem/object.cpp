#include <string.h>
#include "object.h"
#include <system_calls.h>
#include <services/virtualFileSystem/virtualFileSystem.h>

using namespace VirtualFileSystem;
using namespace Vostok;

namespace PFS {

    ProcessObject::ProcessObject(uint32_t pid)
        : pid{pid} {
        memset(executable, '\0', sizeof(executable));
    }

    void ProcessObject::readSelf(uint32_t requesterTaskId, uint32_t requestId) {
        ReadResult result;
        result.success = true;
        result.requestId = requestId;
        ArgBuffer args {result.buffer, sizeof(result.buffer)};
        args.writeType(ArgTypes::Property);

        args.writeValueWithType("executable", ArgTypes::Cstring);

        args.writeType(ArgTypes::EndArg);

        result.recipientId = requesterTaskId;
        send(IPC::RecipientType::TaskId, &result);
    }

    /*
    Vostok Object function support
    */
    int ProcessObject::getFunction(std::string_view name) {
        if (name.compare("testA") == 0) {
            return static_cast<int>(FunctionId::TestA);
        }
        else if (name.compare("testB") == 0) {
            return static_cast<int>(FunctionId::TestB);
        }

        return -1;
    }

    void ProcessObject::readFunction(uint32_t requesterTaskId, uint32_t requestId, uint32_t functionId) {
        describeFunction(requesterTaskId, requestId, functionId);
    }

    void replyWriteSucceeded(uint32_t requesterTaskId, uint32_t requestId, bool success) {
        WriteResult result;
        result.requestId = requestId;
        result.success = success;
        result.recipientId = requesterTaskId;
        send(IPC::RecipientType::TaskId, &result);
    }

    void ProcessObject::writeFunction(uint32_t requesterTaskId, uint32_t requestId, uint32_t functionId, ArgBuffer& args) {
        auto type = args.readType();

        if (type != ArgTypes::Function) {
            replyWriteSucceeded(requesterTaskId, requestId, false);
            return;
        }

        switch(functionId) {
            case static_cast<uint32_t>(FunctionId::TestA): {

                auto x = args.read<uint32_t>(ArgTypes::Uint32);

                if (!args.hasErrors()) {
                    testA(requesterTaskId, x);
                }
                else {
                    replyWriteSucceeded(requesterTaskId, requestId, false);
                }

                break;
            }
            case static_cast<uint32_t>(FunctionId::TestB): {

                auto b = args.read<bool>(ArgTypes::Bool);

                if (!args.hasErrors()) {
                    testB(requesterTaskId, b);
                }
                else {
                    replyWriteSucceeded(requesterTaskId, requestId, false);
                }

                break;
            }
            default: {
                replyWriteSucceeded(requesterTaskId, requestId, false);
            }
        }
    }

    void ProcessObject::describeFunction(uint32_t requesterTaskId, uint32_t requestId, uint32_t functionId) {
        ReadResult result {};
        result.requestId = requestId;
        result.success = true;
        ArgBuffer args{result.buffer, sizeof(result.buffer)};
        args.writeType(ArgTypes::Function);
        
        switch(functionId) {
            case static_cast<uint32_t>(FunctionId::TestA): {
                args.writeType(ArgTypes::Uint32);
                args.writeType(ArgTypes::Void);
                args.writeType(ArgTypes::EndArg);
                break;
            }
            case static_cast<uint32_t>(FunctionId::TestB): {
                args.writeType(ArgTypes::Bool);
                args.writeType(ArgTypes::Void);
                args.writeType(ArgTypes::EndArg);
                break;
            }
            default: {
                result.success = false;
            }
        }

        result.recipientId = requesterTaskId;
        send(IPC::RecipientType::TaskId, &result);
    }

    /*
    Vostok Object property support
    */
    int ProcessObject::getProperty(std::string_view name) {
        if (name.compare("executable") == 0) {
            return static_cast<int>(PropertyId::Executable);
        }

        return -1;
    }

    void ProcessObject::readProperty(uint32_t requesterTaskId, uint32_t requestId, uint32_t propertyId) {
        ReadResult result;
        result.requestId = requestId;
        result.success = true;
        ArgBuffer args{result.buffer, sizeof(result.buffer)};
        args.writeType(ArgTypes::Property);

        switch(static_cast<PropertyId>(propertyId)) {
            case PropertyId::Executable: {
                args.writeValueWithType(executable, ArgTypes::Cstring);
                break;
            }
        }

        args.writeType(ArgTypes::EndArg);

        result.recipientId = requesterTaskId;
        send(IPC::RecipientType::TaskId, &result);
    }

    void ProcessObject::writeProperty(uint32_t requesterTaskId, uint32_t requestId, uint32_t propertyId, ArgBuffer& args) {
        auto type = args.readType();

        if (type != ArgTypes::Property) {
            replyWriteSucceeded(requesterTaskId, requestId, false);
            return;
        }

        switch(static_cast<PropertyId>(propertyId)) {
            case PropertyId::Executable: {

                auto x = args.read<char*>(ArgTypes::Cstring);

                if (!args.hasErrors()) {
                    memcpy(executable, x, sizeof(executable));
                    replyWriteSucceeded(requesterTaskId, requestId, true);
                }
                else {
                    replyWriteSucceeded(requesterTaskId, requestId, false);
                }

                break;
            } 
            default: {
                replyWriteSucceeded(requesterTaskId, requestId, false);
            }
        }
    }

    Object* ProcessObject::getNestedObject(std::string_view) {
        return nullptr;
    }

    /*
    ProcessObject specific implementation
    */
    void ProcessObject::testA(uint32_t requesterTaskId, int x) {
        replyWriteSucceeded(requesterTaskId, 0, false);
        ReadResult result;
        result.success = true;
        char s[10];
        memset(s, '\0', sizeof(s));
        sprintf(s, "testA: %d", x);
        memcpy(result.buffer, s, strlen(s));
        result.recipientId = requesterTaskId;
        send(IPC::RecipientType::TaskId, &result);
    }

    void ProcessObject::testB(uint32_t requesterTaskId, bool b) {
        replyWriteSucceeded(requesterTaskId, 0, false);
        ReadResult result;
        result.success = true;
        char s[10];
        memset(s, '\0', sizeof(s));
        sprintf(s, "testB: %d", b);
        memcpy(result.buffer, s, strlen(s));
        result.recipientId = requesterTaskId;
        send(IPC::RecipientType::TaskId, &result);
    }
}