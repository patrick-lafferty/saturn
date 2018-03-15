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
#include <string_view>
#include <stack>
#include <vector>
#include <list>
#include "cache.h"
#include "messages.h"

namespace Kernel {
    struct ShareMemoryInvitation;
    struct ShareMemoryResult;
}

namespace VirtualFileSystem {

    struct PendingOpen {
        Cache::Entry* entry;
        Cache::Directory* parent;
        char* fullPath;
        std::string_view remainingPath;
        bool completed {false};
        std::stack<Cache::Entry*, std::vector<Cache::Entry*>> breadcrumbs;
    };

    struct PendingCreate {
        Cache::Entry* entry;
        Cache::Directory* parent;
        char* fullPath;
        std::string_view remainingPath;
        bool completed {false};
        std::stack<Cache::Entry*, std::vector<Cache::Entry*>> breadcrumbs;
    };

    struct PendingBlock {
        uint32_t index;
        bool finishedReading {false};
    };

    struct PendingRead {
        int remainingBlocks;
        int currentBlock;
        PendingBlock blocks[2];
        uint32_t filePosition;
        uint32_t readLength;
    };

    struct PendingStream {
        uint32_t startingFilePosition;
        uint32_t readLength;
        std::vector<PendingBlock> blocks;
        int remainingBlocks;
        int currentBlock {0};
        uint8_t* buffer {nullptr};
        bool bufferShareWasSuccessful {false};
        uint32_t pageOffset;
    };

    enum class RequestType {
        Open,
        Create,
        Read,
        Write,
        Seek,
        Stream
    };

    struct PendingRequest {

        uint32_t id;
        uint32_t requesterTaskId;
        RequestType type;
        uint32_t virtualFileDescriptor;

            PendingOpen open;
            PendingOpen create;
            PendingRead read;
            PendingStream stream;

    };

    struct VirtualFileDescriptor {
        uint32_t descriptor;
        uint32_t mountTaskId;
        Cache::Entry* entry;
        uint32_t filePosition;

        bool isOpen() const {
            return mountTaskId != 0;
        }

        void close() {
            descriptor = 0;
            mountTaskId = 0;
        }
    };

    struct MountObserver {
        char path[64];
        std::vector<uint32_t> subscribers;
    };

    class VirtualFileSystem {
    public:

        VirtualFileSystem();
        void messageLoop();

    private:
        
        void handleMountRequest(MountRequest& request);
        void handleGetDirectoryEntriesResult(GetDirectoryEntriesResult& result);
        void handleOpenRequest(OpenRequest& request);
        bool tryOpen(Cache::Entry* entry, PendingRequest& pendingRequest);
        void handleOpenResult(OpenResult& result);
        void handleCreateRequest(CreateRequest& request);
        bool tryCreate(Cache::Entry* entry, PendingRequest& pendingRequest);
        void handleCreateResult(CreateResult& result);
        void handleReadRequest(ReadRequest& request);
        void handleReadResult(ReadResult& result);
        void handleRead512Result(Read512Result& result);
        void handleReadStreamRequest(ReadStreamRequest& request);
        void handleWriteRequest(WriteRequest& request);
        void handleWriteResult(WriteResult& result);
        void handleCloseRequest(CloseRequest& request);
        void handleSeekRequest(SeekRequest& request);
        void handleSeekResult(SeekResult& request);
        void handleSubscribeMount(SubscribeMount& request);

        void handleShareMemoryInvitation(Kernel::ShareMemoryInvitation& invitation);
        void handleShareMemoryResult(Kernel::ShareMemoryResult& result);

        void readDirectoryFromCache(ReadRequest& request, VirtualFileDescriptor& descriptor);
        void readFileFromCache(ReadRequest& request, VirtualFileDescriptor& descriptor);
        uint32_t getNextRequestId();

        uint32_t nextId;

        Cache::Union root;
        std::list<PendingRequest> pendingRequests;
        std::vector<VirtualFileDescriptor> openFileDescriptors;
        std::list<MountObserver> mountObservers;

    };

    void completeReadStreamRequest(PendingRequest& request, VirtualFileDescriptor& descriptor);

    void service();
}