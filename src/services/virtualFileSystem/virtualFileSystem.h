#pragma once

#include <stdint.h>
#include <ipc.h>
#include <string.h>
#include <string_view>
#include <stack>
#include <vector>
#include "cache.h"
#include "messages.h"

namespace VirtualFileSystem {

    class VirtualFileSystem {
    public:

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
        void handleWriteRequest(WriteRequest& request);
        void handleWriteResult(WriteResult& result);
        void handleCloseRequest(CloseRequest& request);

        Cache::Union root;
        std::vector<PendingRequest> pendingRequests;
        uint32_t nextRequestId;
        std::vector<FileDescriptor> openFileDescriptors;
    };

    void service();

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

    enum class RequestType {
        Open,
        Create,
        Read,
        Write
    };

    struct PendingRequest {

        PendingRequest() {

        }

        union {
            PendingOpen open;
            PendingOpen create;
        };

        uint32_t id;
        uint32_t requesterTaskId;
        RequestType type;
    };

}