#pragma once

#include <stdint.h>
#include <ipc.h>

namespace VFS {

    void service();

    class MountService {

    };

    class Object {
    public:
        virtual void read(uint32_t requesterTaskId, uint32_t functionId) {}
    };

    struct MountRequest : IPC::Message {
        MountRequest() {
            messageId = MessageId;
            length = sizeof(MountRequest);
        }

        static uint32_t MessageId;
        char* path;
        uint32_t serviceId;
    };

    //TODO: result or response?
    //mountresult

    struct OpenRequest : IPC::Message {
        OpenRequest() {
            messageId = MessageId;
            length = sizeof(OpenRequest);
        }

        static uint32_t MessageId;
        char* path;

        //TODO: read/write, permissions
    };

    struct OpenResult : IPC::Message {
        OpenResult() {
            messageId = MessageId;
            length = sizeof(OpenResult);
        }

        static uint32_t MessageId;
        uint32_t fileDescriptor;
        bool success;
    };

    struct ReadRequest : IPC::Message {
        ReadRequest() {
            messageId = MessageId;
            length = sizeof(ReadRequest);
        }

        static uint32_t MessageId;
        uint32_t fileDescriptor;
        uint32_t readLength;
    };

    struct ReadResult : IPC::Message {
        ReadResult() {
            messageId = MessageId;
            length = sizeof(ReadResult);
        }

        static uint32_t MessageId;
        bool success;
        uint8_t buffer[256];
    };

}