#include "processFileSystem.h"
#include <services.h>
#include <system_calls.h>
#include <string.h>

using namespace Kernel;
using namespace VFS;

namespace PFS {

    void registerMessages() {

    }

    struct FileDescriptor {
        Object* instance;
        uint32_t functionId;

        void read(uint32_t requesterTaskId) {
            instance->read(requesterTaskId, functionId);
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
        switch(functionId) {
            case static_cast<uint32_t>(FunctionId::TestA): {
                testA(requesterTaskId);
                break;
            }
            case static_cast<uint32_t>(FunctionId::TestB): {
                testB(requesterTaskId);
                break;
            }
            default: {
                ReadResult result {};
                result.success = false;
                result.recipientId = requesterTaskId;
                send(IPC::RecipientType::TaskId, &result);
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

    void ProcessObject::testA(uint32_t requesterTaskId) {
        ReadResult result {};
        result.success = true;
        char* s = "testA";
        memcpy(result.buffer, s, strlen(s));
        result.recipientId = requesterTaskId;
        send(IPC::RecipientType::TaskId, &result);
    }

    void ProcessObject::testB(uint32_t requesterTaskId) {
        ReadResult result {};
        result.success = true;
        char* s = "testB";
        memcpy(result.buffer, s, strlen(s));
        result.recipientId = requesterTaskId;
        send(IPC::RecipientType::TaskId, &result);
    }
}