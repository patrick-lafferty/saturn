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
                //TODO: actual implementation
                OpenResult result{};
                result.success = true;
                result.serviceType = ServiceType::VFS;
                result.fileDescriptor = nextFileDescriptor++;

                send(IPC::RecipientType::ServiceName, &result);
            }
            else if (buffer.messageId == ReadRequest::MessageId) {
                auto request = IPC::extractMessage<ReadRequest>(buffer);
                openDescriptors[0].read(request.senderTaskId);
            }
        }
    }

    void service() {
        sleep(1000);
        MountRequest request{};
        char* path = "/process";
        request.path = new char[strlen(path) + 1];
        memcpy(request.path, path, strlen(request.path) + 1);
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
        }
    }

    void ProcessObject::testA(uint32_t requesterTaskId) {
        ReadResult result {};
        result.success = true;
        char* s = "testA";
        memcpy(result.buffer, s, strlen(s));
        result.recipientId = requesterTaskId;
        send(IPC::RecipientType::TaskId, &result);
    }
}