#include "virtualFileSystem.h"
#include <services.h>
#include <system_calls.h>
#include <string.h>
#include <vector>
#include <parsing>

using namespace Kernel;

namespace VirtualFileSystem {

    uint32_t MountRequest::MessageId;
    uint32_t GetDirectoryEntries::MessageId;
    uint32_t GetDirectoryEntriesResult::MessageId;
    uint32_t OpenRequest::MessageId;
    uint32_t OpenResult::MessageId;
    uint32_t CreateRequest::MessageId;
    uint32_t CreateResult::MessageId;
    uint32_t ReadRequest::MessageId;
    uint32_t ReadResult::MessageId;
    uint32_t WriteRequest::MessageId;
    uint32_t WriteResult::MessageId;
    uint32_t CloseRequest::MessageId;
    uint32_t CloseResult::MessageId;

    void registerMessages() {
        IPC::registerMessage<MountRequest>();
        IPC::registerMessage<GetDirectoryEntries>();
        IPC::registerMessage<GetDirectoryEntriesResult>();
        IPC::registerMessage<OpenRequest>();
        IPC::registerMessage<OpenResult>();
        IPC::registerMessage<CreateRequest>();
        IPC::registerMessage<CreateResult>();
        IPC::registerMessage<ReadRequest>();
        IPC::registerMessage<ReadResult>();
        IPC::registerMessage<WriteRequest>();
        IPC::registerMessage<WriteResult>();
        IPC::registerMessage<CloseRequest>();
        IPC::registerMessage<CloseResult>();
    }

    struct Mount {
        char* path;
        uint32_t pathLength;
        uint32_t serviceId;

        bool exists() {
            return path != nullptr;
        }
    };

    struct FileDescriptor {
        uint32_t descriptor;
        uint32_t mountTaskId;

        bool isOpen() const {
            return mountTaskId != 0;
        }

        void close() {
            descriptor = 0;
            mountTaskId = 0;
        }
    };

    int findEmptyDescriptor(std::vector<FileDescriptor>& descriptors) {
        if (descriptors.empty()) {
            return -1;
        }
        else {
            int i = 0;
            for (auto& desc : descriptors) {
                if (!desc.isOpen()) {
                    return i;
                }

                i++;
            }
        }

        return -1;
    }

    bool findMount(std::vector<Mount>& mounts, char* path, Mount& matchingMount) {
        for (auto& mount : mounts) {

            if (!mount.exists()) {
                continue;
            }

            if (strncmp(path, mount.path, mount.pathLength) == 0 && mount.pathLength <= strlen(path)) {
                matchingMount = mount;
                return true;
            }
        }

        return false;
    }

    /*struct PendingRequest {
        uint32_t id;
        uint32_t requesterTaskId;
    };*/

    PendingRequest* getPendingRequest(uint32_t requestId, std::vector<PendingRequest>& requests) {
        //PendingRequest result;

        for (auto& request : requests) {
            if (request.id == requestId) {
                //result = request;
                return &request;
                //requests.erase(&request);
                //break;
            }
        }

        //return result;
        return nullptr;
    }

    Cache::Directory* createDummyDirectory(MountRequest& request, std::string_view path) {
        auto directory = new Cache::Directory();
        directory->mount = request.senderTaskId;
        directory->cacheable = request.cacheable;
        directory->needsRead = request.cacheable;
        directory->writeable = request.writeable;
        path.copy(directory->path, path.length());
        directory->path[path.length()] = '\0';

        return directory;
    }

    void VirtualFileSystem::handleMountRequest(MountRequest& request) {
        std::string_view path {request.path, strlen(request.path)};
        auto subpaths = split(path, '/');
        Cache::Entry* currentDirectory = &root;
        auto lastPath = subpaths.size();

        auto makeDummyDirectory = [&](auto entry, auto subpath) {
            auto directory = createDummyDirectory(request, subpath);
            entry->children.push_back(directory);
            currentDirectory = directory;
        };

        for(auto i = 0u; i < lastPath; i++) {
            auto subpath = subpaths[i];
            bool isLast = i == (lastPath - 1);

            switch (currentDirectory->type) {
                case Cache::Type::Directory: {

                    /*
                    if we're cacheable/writeable and this dir is, add an entry
                    else, replace with union, make new directory

                    hfs creates uncacheable dummy /system/hardware
                        /system becomes an uncacheable dummy directory

                    ext2fs wants to create /system/configuration
                    */
                    auto entry = static_cast<Cache::Directory*>(currentDirectory);

                    if (entry->children.empty()) {
                        makeDummyDirectory(entry, subpath);
                    }
                    else {
                        bool found {false};

                        for (auto j = 0; j < entry->children.size(); j++) {
                            auto child = entry->children[j];

                            if (subpath.compare(child->path) == 0) {
                                if (isLast) {
                                    if (child->type == Cache::Type::Union) {
                                        makeDummyDirectory(static_cast<Cache::Union*>(child), subpath);
                                    }
                                    else if (child->type == Cache::Type::Directory) {
                                        auto replacementUnion = new Cache::Union();
                                        replacementUnion->children.push_back(static_cast<Cache::Directory*>(child));
                                        makeDummyDirectory(replacementUnion, subpath);
                                        entry->children[j] = replacementUnion;
                                        found = true;
                                    }
                                    else {
                                        //TODO:: error?
                                    }
                                }
                                else {
                                    currentDirectory = child;
                                    found = true;
                                    break;
                                }
                            }
                        }

                        if (!found) {
                            makeDummyDirectory(entry, subpath);
                        }
                    }

                    break;
                }
                case Cache::Type::Union: {
                    auto entry = static_cast<Cache::Union*>(currentDirectory);
                    makeDummyDirectory(entry, subpath);

                    break;
                }
                case Cache::Type::File: {
                    //TODO: error?
                    break;
                }
            }
        }
    }

    void VirtualFileSystem::handleGetDirectoryEntriesResult(GetDirectoryEntriesResult& result) {
        auto pendingRequest = getPendingRequest(result.requestId, pendingRequests);
        auto ptr = result.data;
        auto directory = static_cast<Cache::Directory*>(pendingRequest->open.entry);

        for (auto i = 0u; i < sizeof(result.data); i++) {
            auto index = *reinterpret_cast<uint32_t*>(ptr);
            ptr += 4;
            auto type = static_cast<Cache::Type>(*ptr);
            ptr++;
            i += 5;
            auto path = reinterpret_cast<char*>(ptr);
            auto pathLength = strlen(path);

            Cache::Entry* entry;
            entry->cacheable = directory->cacheable;
            entry->needsRead = directory->cacheable;
            entry->writeable = directory->writeable;

            switch(type) {
                case Cache::Type::File: {
                    entry = new Cache::File();
                    break;
                }
                case Cache::Type::Directory: {
                    auto dir = new Cache::Directory();
                    dir->mount = directory->mount;
                    entry = dir;
                    break;
                }
            }
            
            entry->type = static_cast<Cache::Type>(type);
            memcpy(entry->path, path, pathLength);
            entry->index = index;

            directory->children.push_back(entry);

            ptr += pathLength;
            i += pathLength;
        }

        if (result.expectMore) {
            return;
        }
                
        switch(pendingRequest->type) {
            case RequestType::Open: {
                if (!tryOpen(directory, *pendingRequest)) {
                    OpenResult result;
                    result.recipientId = pendingRequest->requesterTaskId;
                    result.success = false;
                    send(IPC::RecipientType::TaskId, &result); 
                }
                break;
            }
            case RequestType::Create: {
                if (!tryCreate(directory, *pendingRequest)) {
                    CreateResult result;
                    result.recipientId = pendingRequest->requesterTaskId;
                    result.success = false;
                    send(IPC::RecipientType::TaskId, &result); 
                } 
                break;
            }
            default: {
                //read/write never depend on GetDirectoryEntries,
                //so this should never get here
            }
        }
    }

    enum class DiscoverResult {
        DependsOnRead,
        DependsOnMount,
        Failed,
        Succeeded
    };

    DiscoverResult discoverPath(Cache::Entry* currentDirectory, PendingOpen& pending, bool skipLast = false) {

        auto subpaths = split(pending.remainingPath, '/');
        auto lastPath = subpaths.size();

        if (skipLast) {
            lastPath--;
        }

        for (auto i = 0u; i < lastPath; i++) {
            auto subpath = subpaths[i];
            bool isLast = i == (lastPath - 1);

            switch(currentDirectory->type) {
                case Cache::Type::File: {
                    if (!isLast) {
                        return DiscoverResult::Failed;
                    }
                    else {
                        if (subpath.compare(currentDirectory->path) == 0) {
                            pending.entry = currentDirectory;
                            return DiscoverResult::Succeeded;
                        }
                        else {
                            return DiscoverResult::Failed;
                        }
                    }

                    break;
                }
                case Cache::Type::Directory: {

                    auto dir = static_cast<Cache::Directory*>(currentDirectory);
                    if (dir->needsRead) {
                        /*
                        we don't know the contents, need to first send a
                        readDirectory message to the appropriate system
                        */
                        pending.entry = dir;
                        return DiscoverResult::DependsOnRead;
                    }
                    else if (dir->cacheable) {

                        for (auto child : dir->children) {
                            if (subpath.compare(child->path) == 0) {
                                if (isLast) {
                                    pending.entry = child;
                                    return DiscoverResult::Succeeded;
                                }
                                else {
                                    currentDirectory = child;
                                    pending.parent = dir;
                                    continue;
                                }
                            }
                        }

                        //if we got here then there is no such file
                        return DiscoverResult::Failed;
                    }
                    else {
                        /*
                        as far as the VFS is concerned, this directory is opaque.
                        the owning filesystem needs to take over from here.
                        */
                        pending.entry = dir;
                        pending.remainingPath.remove_prefix(subpath.length());
                        return DiscoverResult::DependsOnMount;
                    }

                    break;
                }
                case Cache::Type::Union: {

                    

                    break;
                }
            }
        } 

        return DiscoverResult::Failed;
    }

    void VirtualFileSystem::handleOpenRequest(OpenRequest& request) {
        std::string_view path {request.path, strlen(request.path)};
        Cache::Entry* currentDirectory = &root;

        PendingOpen pendingOpen{};
        pendingOpen.remainingPath = path;
        pendingOpen.fullPath = nullptr;

        auto requestId = nextRequestId++;    
        PendingRequest pendingRequest;
        pendingRequest.open = pendingOpen;
        pendingRequest.type = RequestType::Open;
        pendingRequest.id = requestId;
        pendingRequest.requesterTaskId = request.senderTaskId;

        if (!tryOpen(currentDirectory, pendingRequest)) {
            OpenResult result;
            result.recipientId = request.senderTaskId;
            result.success = false;
            send(IPC::RecipientType::TaskId, &result); 
        }
    }

    bool VirtualFileSystem::tryOpen(Cache::Entry* currentDirectory, PendingRequest& pendingRequest) {

        auto& pendingOpen = pendingRequest.open;
        auto pathStart = pendingOpen.remainingPath.data();
        auto result = discoverPath(currentDirectory, pendingOpen);
        auto requestId = pendingRequest.id;
        
        auto savePath = [&]() {
            if (pendingOpen.fullPath != nullptr) {
                return;
            }

            auto path = pendingOpen.remainingPath;

            pendingOpen.fullPath = new char[path.length()];
            path.copy(pendingOpen.fullPath, path.length());
            auto offset = pendingOpen.remainingPath.data() - pathStart;
            pendingOpen.remainingPath = std::string_view{pendingOpen.fullPath + offset, pendingOpen.remainingPath.length()};
        };

        switch(result) {

            case DiscoverResult::Succeeded: {
                OpenRequest request;
                request.recipientId = pendingOpen.parent->mount;
                request.requestId = pendingRequest.id; 

                send(IPC::RecipientType::TaskId, &request);                

                break;
            }
            case DiscoverResult::DependsOnRead: {
                savePath();

                GetDirectoryEntries getRequest;
                getRequest.requestId = requestId;

                auto directory = static_cast<Cache::Directory*>(pendingOpen.entry);
                getRequest.index = directory->index;
                getRequest.recipientId = directory->mount;

                send(IPC::RecipientType::TaskId, &getRequest);

                break;
            }
            case DiscoverResult::DependsOnMount: {
                savePath();

                auto directory = static_cast<Cache::Directory*>(pendingOpen.entry);

                OpenRequest request;
                request.recipientId = directory->mount;
                request.requestId = requestId;

                send(IPC::RecipientType::TaskId, &request);   
                break;
            }
            case DiscoverResult::Failed: {
                
                return false;
            }
        }

        pendingRequests.push_back(pendingRequest);

        return true;
    }
    
    void VirtualFileSystem::handleOpenResult(OpenResult& result) {
        auto request = getPendingRequest(result.requestId, pendingRequests);
        auto& pendingRequest = *request;

        if (result.success) {
            auto processFileDescriptor = findEmptyDescriptor(openFileDescriptors);
            if (processFileDescriptor >= 0) {                            
                openFileDescriptors[processFileDescriptor] = {
                    result.fileDescriptor,
                    pendingRequest.open.parent->mount
                };
            }
            else {
                processFileDescriptor = openFileDescriptors.size();
                openFileDescriptors.push_back({
                    result.fileDescriptor,
                    pendingRequest.open.parent->mount
                });
            }

            result.fileDescriptor = processFileDescriptor;
            result.recipientId = pendingRequest.requesterTaskId;

            pendingRequests.erase(request);

            send(IPC::RecipientType::TaskId, &result);

        }
        else {

            if (!pendingRequest.open.breadcrumbs.empty()) {
                auto& open = pendingRequest.open;
                auto nextEntry = open.breadcrumbs.top();

                while (!open.breadcrumbs.empty()) {
                    open.breadcrumbs.pop();

                    if (tryOpen(nextEntry, pendingRequest)) {
                        return;
                    }

                    nextEntry = open.breadcrumbs.top();
                }
            }

            result.recipientId = pendingRequest.requesterTaskId;
            pendingRequests.erase(request);
            send(IPC::RecipientType::TaskId, &result); 
        }
    }

    bool VirtualFileSystem::tryCreate(Cache::Entry* currentDirectory, PendingRequest& pendingRequest) {

        auto& pendingCreate = pendingRequest.create;
        auto pathStart = pendingCreate.remainingPath.data();
        auto result = discoverPath(currentDirectory, pendingCreate);
        auto requestId = pendingRequest.id;
        
        auto savePath = [&]() {
            if (pendingCreate.fullPath != nullptr) {
                return;
            }

            auto path = pendingCreate.remainingPath;

            pendingCreate.fullPath = new char[path.length()];
            path.copy(pendingCreate.fullPath, path.length());
            auto offset = pendingCreate.remainingPath.data() - pathStart;
            pendingCreate.remainingPath = std::string_view{pendingCreate.fullPath + offset, pendingCreate.remainingPath.length()};
        };

        if (pendingCreate.entry->type != Cache::Type::Directory) {
            return false;
        }

        auto parentDirectory = static_cast<Cache::Directory*>(pendingCreate.entry);

        switch(result) {

            case DiscoverResult::Succeeded: {
                CreateRequest request;
                request.recipientId = parentDirectory->mount;
                request.requestId = pendingRequest.id; 

                send(IPC::RecipientType::TaskId, &request);                

                break;
            }
            case DiscoverResult::DependsOnRead: {
                savePath();

                GetDirectoryEntries getRequest;
                getRequest.requestId = requestId;

                auto directory = static_cast<Cache::Directory*>(pendingCreate.entry);
                getRequest.index = directory->index;
                getRequest.recipientId = directory->mount;

                send(IPC::RecipientType::TaskId, &getRequest);

                break;
            }
            case DiscoverResult::DependsOnMount: {
                savePath();

                auto directory = static_cast<Cache::Directory*>(pendingCreate.entry);

                CreateRequest request;
                request.recipientId = directory->mount;
                request.requestId = requestId;

                send(IPC::RecipientType::TaskId, &request);   
                break;
            }
            case DiscoverResult::Failed: {
                
                return false;
            }
        }

        pendingRequests.push_back(pendingRequest);

        return true;
    }

    void VirtualFileSystem::handleCreateRequest(CreateRequest& request) {
        std::string_view path {request.path, strlen(request.path)};
        Cache::Entry* currentDirectory = &root;

        PendingOpen pendingCreate{};
        pendingCreate.remainingPath = path;
        pendingCreate.fullPath = nullptr;

        auto requestId = nextRequestId++;    
        PendingRequest pendingRequest;
        pendingRequest.create = pendingCreate;
        pendingRequest.type = RequestType::Create;
        pendingRequest.id = requestId;
        pendingRequest.requesterTaskId = request.senderTaskId;

        if (!tryCreate(currentDirectory, pendingRequest)) {
            CreateResult result;
            result.recipientId = request.senderTaskId;
            result.success = false;
            send(IPC::RecipientType::TaskId, &result); 
        } 
    }

    void VirtualFileSystem::handleCreateResult(CreateResult& result) {
        auto request = getPendingRequest(result.requestId, pendingRequests);
        auto& pendingRequest = *request;

        if (result.success) {
            result.recipientId = pendingRequest.requesterTaskId;
            send(IPC::RecipientType::TaskId, &result);
        }
        else {

            if (!pendingRequest.create.breadcrumbs.empty()) {
                auto& create = pendingRequest.create;
                auto nextEntry = create.breadcrumbs.top();

                while (!create.breadcrumbs.empty()) {
                    create.breadcrumbs.pop();

                    if (tryCreate(nextEntry, pendingRequest)) {
                        return;
                    }

                    nextEntry = create.breadcrumbs.top();
                }
            }

            result.recipientId = pendingRequest.requesterTaskId;
            pendingRequests.erase(request);
            send(IPC::RecipientType::TaskId, &result); 
        }
    }

    void VirtualFileSystem::handleReadRequest(ReadRequest& request) {
        bool failed {false};
        if (request.fileDescriptor < openFileDescriptors.size()) {
            auto descriptor = openFileDescriptors[request.fileDescriptor];

            if (descriptor.isOpen()) {
                request.requestId = nextRequestId++;
                PendingRequest pending;
                pending.type = RequestType::Read;
                pending.id = request.requestId;
                pending.requesterTaskId = request.senderTaskId;
                pendingRequests.push_back(pending);
                request.recipientId = descriptor.mountTaskId;
                request.fileDescriptor = descriptor.descriptor;
                send(IPC::RecipientType::TaskId, &request);
            }
            else {
                failed = true;
            }
        }
        else {
            failed = true;
        }

        if (failed) {
            ReadResult result;
            result.success = false;
            result.recipientId = request.senderTaskId;
            send(IPC::RecipientType::TaskId, &result);
        }
    }

    void VirtualFileSystem::handleReadResult(ReadResult& result) {
        auto pendingRequest = getPendingRequest(result.requestId, pendingRequests);
        result.recipientId = pendingRequest->requesterTaskId;
        pendingRequests.erase(pendingRequest);
        send(IPC::RecipientType::TaskId, &result);
    }

    void VirtualFileSystem::handleWriteRequest(WriteRequest& request) {
        bool failed {false};

        if (request.fileDescriptor < openFileDescriptors.size()) {
            auto descriptor = openFileDescriptors[request.fileDescriptor];

            if (descriptor.isOpen()) {
                request.requestId = nextRequestId++;
                PendingRequest pending;
                pending.type = RequestType::Write;
                pending.id = request.requestId;
                pending.requesterTaskId = request.senderTaskId;
                pendingRequests.push_back(pending);
                request.recipientId = descriptor.mountTaskId;
                request.fileDescriptor = descriptor.descriptor;
                send(IPC::RecipientType::TaskId, &request);
            }
            else {
                failed = true;
            }
        }
        else {
            failed = true;
        }

        if (failed) {
            WriteResult result;
            result.success = false;
            result.recipientId = request.senderTaskId;
            send(IPC::RecipientType::TaskId, &result);
        }
    }

    void VirtualFileSystem::handleWriteResult(WriteResult& result) {
        auto pendingRequest = getPendingRequest(result.requestId, pendingRequests);
        result.recipientId = pendingRequest->requesterTaskId;
        pendingRequests.erase(pendingRequest);
        send(IPC::RecipientType::TaskId, &result);
    }

    void VirtualFileSystem::handleCloseRequest(CloseRequest& request) {
        bool failed {false};

        if (request.fileDescriptor < openFileDescriptors.size()) {
            auto& descriptor = openFileDescriptors[request.fileDescriptor];

            if (descriptor.isOpen()) {
                request.fileDescriptor = descriptor.descriptor;
                request.recipientId = descriptor.mountTaskId;
                send(IPC::RecipientType::TaskId, &request);

                descriptor.close();

                CloseResult result;
                result.success = true;
                result.recipientId = request.senderTaskId;
                send(IPC::RecipientType::TaskId, &result);
            }
            else {
                failed = true;
            }
        }
        else {
            failed = true;
        }

        if (failed) {
            CloseResult result;
            result.success = false;
            result.recipientId = request.senderTaskId;
            send(IPC::RecipientType::TaskId, &result);
        }
    }

    void VirtualFileSystem::messageLoop() {
        /*
        TODO: Replace with a trie where the leaf node values are services 
        */
        /*std::vector<Mount> mounts;
        std::vector<FileDescriptor> openFileDescriptors;
        std::vector<PendingRequest> pendingRequests;
        uint32_t nextRequestId {0};
        Cache::Union root;*/

        while (true) {
            IPC::MaximumMessageBuffer buffer;
            receive(&buffer);

            if (buffer.messageId == MountRequest::MessageId) {
                //TODO: for now only support top level mounts to /
                auto request = IPC::extractMessage<MountRequest>(buffer);
                /*auto pathLength = strlen(request.path) + 1;
                mounts.push_back({nullptr, pathLength - 1, request.senderTaskId});
                auto& mount = mounts[mounts.size() - 1];
                mount.path = new char[pathLength];
                memcpy(mount.path, request.path, pathLength);*/
                handleMountRequest(request);

            }
            else if (buffer.messageId == GetDirectoryEntriesResult::MessageId) {
                auto result = IPC::extractMessage<GetDirectoryEntries>(buffer);
                handleGetDirectoryEntriesResult
            }
            else if (buffer.messageId == OpenRequest::MessageId) {
                //TODO: search the trie for the appropriate mount point
                //since this is just an experiment to see if this design works, hardcode mount
                auto request = IPC::extractMessage<OpenRequest>(buffer);
                /*Mount mount;
                auto foundMount = findMount(mounts, request.path, mount);

                if (foundMount) {
                    request.recipientId = mount.serviceId;
                    request.requestId = nextRequestId++;
                    pendingRequests.push_back({request.requestId, buffer.senderTaskId});
                    send(IPC::RecipientType::TaskId, &request);   
                }
                else {
                    OpenResult result;
                    result.recipientId = buffer.senderTaskId;
                    result.success = false;
                    send(IPC::RecipientType::TaskId, &result); 
                }*/
                handleOpenRequest(request);
            }
            else if (buffer.messageId == OpenResult::MessageId) {

                auto result = IPC::extractMessage<OpenResult>(buffer);
                handleOpenResult(result); 

                /*for (auto& mount : mounts) {

                    if (!mount.exists()) {
                        continue;
                    }

                    if (buffer.senderTaskId == mount.serviceId) {
                        auto result = IPC::extractMessage<OpenResult>(buffer);

                        if (result.success) {
                            /*
                            mount services have their own local file descriptors,
                            VFS has its own that encompases all mounts
                            */
                            /*auto processFileDescriptor = findEmptyDescriptor(openFileDescriptors);
                            if (processFileDescriptor >= 0) {                            
                                openFileDescriptors[processFileDescriptor] = {
                                    result.fileDescriptor,
                                    mount.serviceId
                                };
                            }
                            else {
                                processFileDescriptor = openFileDescriptors.size();
                                openFileDescriptors.push_back({
                                    result.fileDescriptor,
                                    mount.serviceId
                                });
                            }

                            result.fileDescriptor = processFileDescriptor;
                            auto pendingRequest = getPendingRequest(result.requestId, pendingRequests);
                            result.recipientId = pendingRequest.requesterTaskId;
                            send(IPC::RecipientType::TaskId, &result);
                        }
                        else {
                            auto pendingRequest = getPendingRequest(result.requestId, pendingRequests);
                            result.recipientId = pendingRequest.requesterTaskId;
                            send(IPC::RecipientType::TaskId, &result);
                        }

                        break;
                    }
                }*/
            }
            else if (buffer.messageId == CreateRequest::MessageId) {
                auto request = IPC::extractMessage<CreateRequest>(buffer);
                handleCreateRequest(request);
                /*Mount mount;
                auto foundMount = findMount(mounts, request.path, mount);

                if (foundMount) {
                    request.recipientId = mount.serviceId;
                    request.requestId = nextRequestId++;
                    send(IPC::RecipientType::TaskId, &request);
                    pendingRequests.push_back({request.requestId, buffer.senderTaskId});
                }
                else {
                    CreateResult result;
                    result.recipientId = request.senderTaskId;
                    result.success = false;
                    send(IPC::RecipientType::TaskId, &result);
                }*/
            }
            else if (buffer.messageId == CreateResult::MessageId) {
                auto result = IPC::extractMessage<CreateResult>(buffer);
                handleCreateResult(result);
                /*auto pendingRequest = getPendingRequest(result.requestId, pendingRequests);
                result.recipientId = pendingRequest.requesterTaskId;
                send(IPC::RecipientType::TaskId, &result);*/
            }
            else if (buffer.messageId == ReadRequest::MessageId) {
                auto request = IPC::extractMessage<ReadRequest>(buffer);
                handleReadRequest(request);
                /*
                TODO: consider implementing either:
                1) message forwarding (could be insecure)
                2) a separate ReadRequestForwarded message
                */
                /*bool failed {false};
                if (request.fileDescriptor < openFileDescriptors.size()) {
                    auto descriptor = openFileDescriptors[request.fileDescriptor];

                    if (descriptor.isOpen()) {
                        request.requestId = nextRequestId++;
                        pendingRequests.push_back({request.requestId, buffer.senderTaskId});
                        request.recipientId = descriptor.mountTaskId;
                        request.fileDescriptor = descriptor.descriptor;
                        send(IPC::RecipientType::TaskId, &request);
                    }
                    else {
                        failed = true;
                    }
                }
                else {
                    failed = true;
                }

                if (failed) {
                    ReadResult result;
                    result.success = false;
                    result.recipientId = request.senderTaskId;
                    send(IPC::RecipientType::TaskId, &result);
                }*/
                
            }
            else if (buffer.messageId == ReadResult::MessageId) {
                auto result = IPC::extractMessage<ReadResult>(buffer);
                handleReadResult(result);
                /*auto pendingRequest = getPendingRequest(result.requestId, pendingRequests);
                result.recipientId = pendingRequest.requesterTaskId;
                send(IPC::RecipientType::TaskId, &result);*/
            }
            else if (buffer.messageId == WriteRequest::MessageId) {
                auto request = IPC::extractMessage<WriteRequest>(buffer);
                handleWriteRequest(request);

                /*bool failed {false};

                if (request.fileDescriptor < openFileDescriptors.size()) {
                    auto descriptor = openFileDescriptors[request.fileDescriptor];

                    if (descriptor.isOpen()) {
                        request.requestId = nextRequestId++;
                        pendingRequests.push_back({request.requestId, buffer.senderTaskId});
                        request.recipientId = descriptor.mountTaskId;
                        request.fileDescriptor = descriptor.descriptor;
                        send(IPC::RecipientType::TaskId, &request);
                    }
                    else {
                        failed = true;
                    }
                }
                else {
                    failed = true;
                }

                if (failed) {
                    WriteResult result;
                    result.success = false;
                    result.recipientId = request.senderTaskId;
                    send(IPC::RecipientType::TaskId, &result);
                }*/
            }
            else if (buffer.messageId == WriteResult::MessageId) {
                auto result = IPC::extractMessage<WriteResult>(buffer);
                handleWriteResult(result);
                /*auto pendingRequest = getPendingRequest(result.requestId, pendingRequests);
                result.recipientId = pendingRequest.requesterTaskId;
                send(IPC::RecipientType::TaskId, &result);*/
            }
            else if (buffer.messageId == CloseRequest::MessageId) {
                auto request = IPC::extractMessage<CloseRequest>(buffer);
                handleCloseRequest(request);

                /*bool failed {false};

                if (request.fileDescriptor < openFileDescriptors.size()) {
                    auto& descriptor = openFileDescriptors[request.fileDescriptor];

                    if (descriptor.isOpen()) {
                        request.fileDescriptor = descriptor.descriptor;
                        request.recipientId = descriptor.mountTaskId;
                        send(IPC::RecipientType::TaskId, &request);

                        descriptor.close();

                        CloseResult result;
                        result.success = true;
                        result.recipientId = buffer.senderTaskId;
                        send(IPC::RecipientType::TaskId, &result);
                    }
                    else {
                        failed = true;
                    }
                }
                else {
                    failed = true;
                }

                if (failed) {
                    CloseResult result;
                    result.success = false;
                    result.recipientId = buffer.senderTaskId;
                    send(IPC::RecipientType::TaskId, &result);
                }*/
            }

        }
    }

    void registerService() {
        RegisterService registerRequest;
        registerRequest.type = ServiceType::VFS;

        IPC::MaximumMessageBuffer buffer;

        send(IPC::RecipientType::ServiceRegistryMailbox, &registerRequest);
        receive(&buffer);

        if (buffer.messageId == GenericServiceMeta::MessageId) {
            registerMessages();
        }

        NotifyServiceReady ready;
        send(IPC::RecipientType::ServiceRegistryMailbox, &ready);
    }

    void service() {
        registerService(); 

        auto system = new VirtualFileSystem();
        system->messageLoop();
    }
}