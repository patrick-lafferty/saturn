#pragma once

#include <stdint.h>
#include <ipc.h>
#include <string.h>

namespace VFS {

    void service();

    struct MountRequest : IPC::Message {
        MountRequest() {
            messageId = MessageId;
            length = sizeof(MountRequest);
            memset(path, '\0', sizeof(path));
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
            memset(path, '\0', sizeof(path));
        }

        static uint32_t MessageId;
        char path[64];
        uint32_t requestId;

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
        uint32_t requestId;
    };

    struct CreateRequest : IPC::Message {
        CreateRequest() {
            messageId = MessageId;
            length = sizeof(CreateRequest);
            memset(path, '\0', sizeof(path));
        }

        static uint32_t MessageId;
        char path[64];
        uint32_t requestId;

        //TODO: read/write, permissions
    };

    struct CreateResult : IPC::Message {
        CreateResult() {
            messageId = MessageId;
            length = sizeof(CreateResult);
        }

        static uint32_t MessageId;
        bool success;
        uint32_t requestId;
    };

    struct ReadRequest : IPC::Message {
        ReadRequest() {
            messageId = MessageId;
            length = sizeof(ReadRequest);
        }

        static uint32_t MessageId;
        uint32_t requestId;
        uint32_t fileDescriptor;
        uint32_t readLength;
    };

    struct ReadResult : IPC::Message {
        ReadResult() {
            messageId = MessageId;
            length = sizeof(ReadResult);
            memset(buffer, 0, sizeof(buffer));
        }

        static uint32_t MessageId;
        uint32_t requestId;
        bool success;
        uint8_t buffer[256];
    };

    struct WriteRequest : IPC::Message {
        WriteRequest() {
            messageId = MessageId;
            length = sizeof(WriteRequest);
            memset(buffer, 0, sizeof(buffer));
        }

        static uint32_t MessageId;
        uint32_t requestId;
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
        uint32_t requestId;
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