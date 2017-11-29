#include "processFileSystem.h"
#include <services.h>
#include <system_calls.h>
#include <string.h>
#include <services/virtualFileSystem/virtualFileSystem.h>
#include <stdio.h>

using namespace Kernel;
using namespace VFS;
using namespace Vostok;

namespace PFS {

    void registerMessages() {

    }

    struct FileDescriptor {
        Object* instance;
        uint32_t functionId;

        void read(uint32_t requesterTaskId) {
            instance->read(requesterTaskId, functionId);
        }

        void write(uint32_t requesterTaskId, ArgBuffer& args) {
            instance->write(requesterTaskId, functionId, args);
        }
    };

    void messageLoop() {

        uint32_t nextFileDescriptor {0};
        FileDescriptor openDescriptors[2];
        ProcessObject p;
        openDescriptors[0].instance = &p;
        openDescriptors[0].functionId = 0;

        while (true) {
            IPC::MaximumMessageBuffer buffer;
            receive(&buffer);

            if (buffer.messageId == OpenRequest::MessageId) {
                auto request = IPC::extractMessage<OpenRequest>(buffer);
                auto functionName = strrchr(request.path, '/');
                bool failed {false};
                OpenResult result{};
                result.serviceType = ServiceType::VFS;

                if (functionName != nullptr) {
                    //TODO: still hardcoded to one instance
                    auto functionId = p.getFunction(functionName + 1);

                    if (functionId >= 0) {
                        openDescriptors[0].functionId = functionId;
                        result.success = true;
                        result.fileDescriptor = nextFileDescriptor++;
                    }
                    else {
                        failed = true;
                    }
                }
                else {
                    failed = true;
                }

                if (failed) {
                    result.success = false;
                }

                send(IPC::RecipientType::ServiceName, &result);
            }
            else if (buffer.messageId == ReadRequest::MessageId) {
                auto request = IPC::extractMessage<ReadRequest>(buffer);
                openDescriptors[0].read(request.senderTaskId);
            }
            else if (buffer.messageId == WriteRequest::MessageId) {
                auto request = IPC::extractMessage<WriteRequest>(buffer);
                ArgBuffer args{request.buffer, sizeof(request.buffer)};
                openDescriptors[0].write(request.senderTaskId, args);
            }
        }
    }

    void service() {

        waitForServiceRegistered(ServiceType::VFS);

        MountRequest request{};
        char* path = "/process";
        memcpy(request.path, path, strlen(path) + 1);
        request.serviceType = ServiceType::VFS;

        send(IPC::RecipientType::ServiceName, &request);

        messageLoop();
    }

    void ProcessObject::read(uint32_t requesterTaskId, uint32_t functionId) {
        describe(requesterTaskId, functionId);
    }

    void replyWriteFailed(uint32_t requesterTaskId) {
        WriteResult result;
        result.success = false;
        result.recipientId = requesterTaskId;
        send(IPC::RecipientType::TaskId, &result);
    }

    void ProcessObject::write(uint32_t requesterTaskId, uint32_t functionId, ArgBuffer& args) {
        switch(functionId) {
            case static_cast<uint32_t>(FunctionId::TestA): {

                auto x = args.read<uint32_t>(ArgTypes::Uint32);

                if (!args.readFailed) {
                    testA(requesterTaskId, x);
                }
                else {
                    replyWriteFailed(requesterTaskId);
                }

                break;
            }
            case static_cast<uint32_t>(FunctionId::TestB): {

                auto b = args.read<bool>(ArgTypes::Bool);

                if (!args.readFailed) {
                    testB(requesterTaskId, b);
                }
                else {
                    replyWriteFailed(requesterTaskId);
                }

                break;
            }
            default: {
                replyWriteFailed(requesterTaskId);
            }
        }
    }

    int ProcessObject::getFunction(char* name) {
        if (strcmp(name, "testA") == 0) {
            return static_cast<int>(FunctionId::TestA);
        }
        else if (strcmp(name, "testB") == 0) {
            return static_cast<int>(FunctionId::TestB);
        }

        return -1;
    }

    void ProcessObject::describe(uint32_t requesterTaskId, uint32_t functionId) {
        ReadResult result {};
        result.success = true;
        ArgBuffer args{result.buffer, sizeof(result.buffer)};
        
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

    void acknowledgeWrite(uint32_t requesterTaskId) {
        WriteResult result;
        result.success = true;
        result.recipientId = requesterTaskId;
        send(IPC::RecipientType::TaskId, &result);
    }

    void ProcessObject::testA(uint32_t requesterTaskId, int x) {
        acknowledgeWrite(requesterTaskId);
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
        acknowledgeWrite(requesterTaskId);
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