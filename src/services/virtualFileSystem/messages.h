/*
Copyright (c) 2017, Patrick Lafferty
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

    * Redistributions of source code must retain the above copyright
      notice, this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright
      notice, this list of conditions and the following disclaimer in the
      documentation and/or other materials provided with the distribution.
    * Neither the name of the copyright holder nor the names of its 
      contributors may be used to endorse or promote products derived from 
      this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR 
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/
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
        ReadStreamRequest,
        ReadStreamResult,
        WriteRequest,
        WriteResult,
        CloseRequest,
        CloseResult,
        SeekRequest,
        SeekResult,
        SyncPositionWithCache,
        SubscribeMount,
        MountNotification
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
        uint32_t index;
        char path[64];

        //TODO: read/write, permissions
    };

    struct OpenResult : IPC::Message {
        OpenResult() {
            messageId = static_cast<uint32_t>(MessageId::OpenResult);
            length = sizeof(OpenResult);
            messageNamespace = IPC::MessageNamespace::VFS;
        }

        uint32_t fileDescriptor;
        uint32_t requestId;
        uint32_t fileLength;
        bool success;
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

    struct ReadStreamRequest : IPC::Message {
        ReadStreamRequest() {
            messageId = static_cast<uint32_t>(MessageId::ReadStreamRequest);
            length = sizeof(ReadStreamRequest);
            messageNamespace = IPC::MessageNamespace::VFS;
        }

        uint32_t requestId;
        uint32_t fileDescriptor;
        uint32_t readLength;
    };

    struct ReadStreamResult : IPC::Message {
        ReadStreamResult() {
            messageId = static_cast<uint32_t>(MessageId::ReadStreamResult);
            length = sizeof(ReadStreamResult);
            bytesWritten = 0;
            messageNamespace = IPC::MessageNamespace::VFS;
        }

        uint32_t requestId;
        bool success;
        uint32_t bytesWritten;
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

    struct SyncPositionWithCache : IPC::Message {
        SyncPositionWithCache() {
            messageId = static_cast<uint32_t>(MessageId::SyncPositionWithCache);
            length = sizeof(SyncPositionWithCache);
            messageNamespace = IPC::MessageNamespace::VFS;
        }

        uint32_t fileDescriptor;
        uint32_t filePosition;
    };

    struct SubscribeMount : IPC::Message {
        SubscribeMount() {
            messageId = static_cast<uint32_t>(MessageId::SubscribeMount);
            length = sizeof(SubscribeMount);
            memset(path, '\0', sizeof(path));
            messageNamespace = IPC::MessageNamespace::VFS;
        }

        char path[64];
    };

    struct MountNotification : IPC::Message {
        MountNotification() {
            messageId = static_cast<uint32_t>(MessageId::MountNotification);
            length = sizeof(MountNotification);
            memset(path, '\0', sizeof(path));
            messageNamespace = IPC::MessageNamespace::VFS;
        }

        char path[64];
    };

}