#pragma once

#include <stdint.h>
#include <ipc.h>

namespace VFS {

    void service();

    struct MountRequest : IPC::Message {
        MountRequest() {
            messageId = MessageId;
            length = sizeof(MountRequest);
        }

        static uint32_t MessageId;
        char path[64];
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
        char path[64];

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

    struct WriteRequest : IPC::Message {
        WriteRequest() {
            messageId = MessageId;
            length = sizeof(WriteRequest);
        }

        static uint32_t MessageId;
        uint32_t fileDescriptor;
        uint32_t writeLength;
        uint8_t buffer[256];
    };

    struct WriteResult : IPC::Message {
        WriteResult() {
            messageId = MessageId;
            length = sizeof(WriteResult);
        }

        static uint32_t MessageId;
        bool success;
    };

    struct CloseRequest : IPC::Message {
        CloseRequest() {
            messageId = MessageId;
            length = sizeof(CloseRequest);
        }

        static uint32_t MessageId;
        uint32_t fileDescriptor;
    };

    struct CloseResult : IPC::Message {
        CloseResult() {
            messageId = MessageId;
            length = sizeof(CloseResult);
        }

        static uint32_t MessageId;
        bool success;
    };

}