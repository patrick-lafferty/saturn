#include "processFileSystem.h"
#include <services.h>
#include <system_calls.h>
#include <string.h>
#include <services/virtualFileSystem/virtualFileSystem.h>
#include <stdio.h>
#include "object.h"

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
}