#pragma once

#include <stdint.h>
#include <ipc.h>

namespace VFS {

    void service();

    class MountService {

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
        uint32_t length;
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