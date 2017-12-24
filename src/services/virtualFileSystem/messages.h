#pragma once

#include <stdint.h>
#include <ipc.h>
#include <string.h>

namespace VirtualFileSystem {

    enum class MessageId {
        MountRequest,
        GetDirectoryEntries,
        GetDirectoryEntriesResult,
        OpenRequest,
        OpenResult,
        CreateRequest,
        CreateResult,
        ReadRequest,
        ReadResult,
        Read512Result,
        WriteRequest,
        WriteResult,
        CloseRequest,
        CloseResult,
        SeekRequest,
        SeekResult
    };

    struct MountRequest : IPC::Message {
        MountRequest() {
            messageId = static_cast<uint32_t>(MessageId::MountRequest);
            length = sizeof(MountRequest);
            memset(path, '\0', sizeof(path));
            messageNamespace = IPC::MessageNamespace::VFS;
        }

        char path[64];
        uint32_t serviceId;
        bool cacheable;
        bool writeable;
    };

    /*
    From VFS to FileSystems only
    */
    struct GetDirectoryEntries : IPC::Message {
        GetDirectoryEntries() {
            messageId = static_cast<uint32_t>(MessageId::GetDirectoryEntries);
            length = sizeof(GetDirectoryEntries);
            messageNamespace = IPC::MessageNamespace::VFS;
        }

        uint32_t requestId;
        uint32_t index;
    };

    struct GetDirectoryEntriesResult : IPC::Message {
        GetDirectoryEntriesResult() {
            messageId = static_cast<uint32_t>(MessageId::GetDirectoryEntriesResult);
            length = sizeof(GetDirectoryEntriesResult);
            messageNamespace = IPC::MessageNamespace::VFS;
        }

        uint32_t requestId;

        /*
        holds at least one entry of the form:
        {
            uint32_t: index
            uint8_t: type
            char*: path (null terminated)
        }
        */
        uint8_t data[262];
        bool expectMore;
    };

    /*
    End VFS to FileSystems only section
    */

    struct OpenRequest : IPC::Message {
        OpenRequest() {
            messageId = static_cast<uint32_t>(MessageId::OpenRequest);
            length = sizeof(OpenRequest);
            memset(path, '\0', sizeof(path));
            messageNamespace = IPC::MessageNamespace::VFS;
        }

        void shrink(int pathLength) {
            length = sizeof(OpenRequest) 
                - sizeof(path)
                + pathLength 
                + sizeof(requestId);
        }

        uint32_t requestId;
        char path[64];
        uint32_t index;

        //TODO: read/write, permissions
    };

    struct OpenResult : IPC::Message {
        OpenResult() {
            messageId = static_cast<uint32_t>(MessageId::OpenResult);
            length = sizeof(OpenResult);
            messageNamespace = IPC::MessageNamespace::VFS;
        }

        uint32_t fileDescriptor;
        bool success;
        uint32_t requestId;
    };

    struct CreateRequest : IPC::Message {
        CreateRequest() {
            messageId = static_cast<uint32_t>(MessageId::CreateRequest);
            length = sizeof(CreateRequest);
            memset(path, '\0', sizeof(path));
            messageNamespace = IPC::MessageNamespace::VFS;
        }

        void shrink(int pathLength) {
            length = sizeof(CreateRequest) 
                - sizeof(path)
                + pathLength 
                + sizeof(requestId);
        } 

        uint32_t requestId;
        char path[64];

        //TODO: read/write, permissions
    };

    struct CreateResult : IPC::Message {
        CreateResult() {
            messageId = static_cast<uint32_t>(MessageId::CreateResult);
            length = sizeof(CreateResult);
            messageNamespace = IPC::MessageNamespace::VFS;
        }

        bool success;
        uint32_t requestId;
    };

    struct ReadRequest : IPC::Message {
        ReadRequest() {
            messageId = static_cast<uint32_t>(MessageId::ReadRequest);
            length = sizeof(ReadRequest);
            messageNamespace = IPC::MessageNamespace::VFS;
        }

        uint32_t requestId;
        uint32_t fileDescriptor;
        uint32_t readLength;
    };

    struct ReadResult : IPC::Message {
        ReadResult() {
            messageId = static_cast<uint32_t>(MessageId::ReadResult);
            length = sizeof(ReadResult);
            memset(buffer, 0, sizeof(buffer));
            bytesWritten = 0;
            messageNamespace = IPC::MessageNamespace::VFS;
        }

        uint32_t requestId;
        bool success;
        uint8_t buffer[256];
        uint32_t bytesWritten;
    };

    struct Read512Result : IPC::Message {
        Read512Result() {
            messageId = static_cast<uint32_t>(MessageId::Read512Result);
            length = sizeof(Read512Result);
            memset(buffer, 0, sizeof(buffer));
            bytesWritten = 0;
            messageNamespace = IPC::MessageNamespace::VFS;
        }

        uint32_t requestId;
        bool success;
        uint8_t buffer[512];
        uint32_t bytesWritten;
        bool expectMore;
    };

    struct WriteRequest : IPC::Message {
        WriteRequest() {
            messageId = static_cast<uint32_t>(MessageId::WriteRequest);
            length = sizeof(WriteRequest);
            memset(buffer, 0, sizeof(buffer));
            messageNamespace = IPC::MessageNamespace::VFS;
        }

        uint32_t requestId;
        uint32_t fileDescriptor;
        uint32_t writeLength;
        uint8_t buffer[256];
    };

    struct WriteResult : IPC::Message {
        WriteResult() {
            messageId = static_cast<uint32_t>(MessageId::WriteResult);
            length = sizeof(WriteResult);
            expectReadResult = false;
            messageNamespace = IPC::MessageNamespace::VFS;
        }

        uint32_t requestId;
        bool success;
        bool expectReadResult;
    };

    struct CloseRequest : IPC::Message {
        CloseRequest() {
            messageId = static_cast<uint32_t>(MessageId::CloseRequest);
            length = sizeof(CloseRequest);
            messageNamespace = IPC::MessageNamespace::VFS;
        }

        uint32_t fileDescriptor;
    };

    struct CloseResult : IPC::Message {
        CloseResult() {
            messageId = static_cast<uint32_t>(MessageId::CloseResult);
            length = sizeof(CloseResult);
            messageNamespace = IPC::MessageNamespace::VFS;
        }

        bool success;
    };

    enum class Origin {
        Beginning,
        Current,
        End
    };

    struct SeekRequest : IPC::Message {
        SeekRequest() {
            messageId = static_cast<uint32_t>(MessageId::SeekRequest);
            length = sizeof(SeekRequest);
            messageNamespace = IPC::MessageNamespace::VFS;
        }

        uint32_t requestId;
        uint32_t fileDescriptor;
        uint32_t offset;
        Origin origin;
    };

    struct SeekResult : IPC::Message {
        SeekResult() {
            messageId = static_cast<uint32_t>(MessageId::SeekResult);
            length = sizeof(SeekResult);
            messageNamespace = IPC::MessageNamespace::VFS;
        }

        uint32_t requestId;
        bool success;
        uint32_t filePosition;
    };

}