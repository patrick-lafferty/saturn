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
    uint32_t Read512Result::MessageId;
    uint32_t WriteRequest::MessageId;
    uint32_t WriteResult::MessageId;
    uint32_t CloseRequest::MessageId;
    uint32_t CloseResult::MessageId;
    uint32_t SeekRequest::MessageId;
    uint32_t SeekResult::MessageId;

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
        IPC::registerMessage<Read512Result>();
        IPC::registerMessage<WriteRequest>();
        IPC::registerMessage<WriteResult>();
        IPC::registerMessage<CloseRequest>();
        IPC::registerMessage<CloseResult>();
        IPC::registerMessage<SeekRequest>();
        IPC::registerMessage<SeekResult>();
    }

    struct Mount {
        char* path;
        uint32_t pathLength;
        uint32_t serviceId;

        bool exists() {
            return path != nullptr;
        }
    };

    VirtualFileSystem::VirtualFileSystem() {
        root.path[0] = '/';
        root.path[1] = '\0';
    }

    int findEmptyDescriptor(std::vector<VirtualFileDescriptor>& descriptors) {
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

    std::list<PendingRequest>::iterator getPendingRequest(uint32_t requestId, std::list<PendingRequest>& requests) {
        for(auto it = requests.begin(); it != requests.end(); ++it) {
            if (it->id == requestId) {
                return it;
            }
        }

        return requests.end();
    }

    Cache::Directory* createDummyMountDirectory(MountRequest& request, std::string_view path) {
        auto directory = new Cache::Directory();
        directory->mount = request.senderTaskId;
        directory->cacheable = request.cacheable;
        directory->needsRead = request.cacheable;
        directory->writeable = request.writeable;
        path.copy(directory->path, path.length());
        directory->path[path.length()] = '\0';

        return directory;
    }

    Cache::Directory* createDummyDirectory(MountRequest& request, std::string_view path) {
        auto directory = new Cache::Directory();
        directory->mount = request.senderTaskId;
        directory->cacheable = true;
        directory->needsRead = false;
        directory->writeable = true;
        path.copy(directory->path, path.length());
        directory->path[path.length()] = '\0';

        return directory;
    }

    void VirtualFileSystem::handleMountRequest(MountRequest& request) {
        std::string_view path {request.path, strlen(request.path)};
        auto subpaths = split(path, '/', true);
        Cache::Entry* currentDirectory = &root;
        auto lastPath = subpaths.size();

        auto makeDummyDirectory = [&](auto entry, auto subpath, bool forMount) {
            Cache::Directory* directory;

            if (forMount) {
                directory = createDummyMountDirectory(request, subpath);
            }
            else {
                directory = createDummyDirectory(request, subpath);

            }

            entry->children.push_back(directory);
            currentDirectory = directory;
        };

        for(auto i = 0u; i < lastPath; i++) {
            auto subpath = subpaths[i];
            bool isLast = i == (lastPath - 1);

            if (i > 0 && subpath.compare("/") == 0) {
                continue;
            }

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
                        makeDummyDirectory(entry, subpath, isLast);
                    }
                    else {
                        bool found {false};

                        for (auto j = 0u; j < entry->children.size(); j++) {
                            auto child = entry->children[j];

                            if (subpath.compare(child->path) == 0) {
                                if (isLast) {
                                    if (child->type == Cache::Type::Union) {
                                        makeDummyDirectory(static_cast<Cache::Union*>(child), subpath, isLast);
                                    }
                                    else if (child->type == Cache::Type::Directory) {
                                        auto replacementUnion = new Cache::Union();
                                        replacementUnion->children.push_back(static_cast<Cache::Directory*>(child));
                                        makeDummyDirectory(replacementUnion, subpath, isLast);
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
                            makeDummyDirectory(entry, subpath, isLast);
                        }
                    }

                    break;
                }
                case Cache::Type::Union: {
                    auto entry = static_cast<Cache::Union*>(currentDirectory);

                    if (entry->children.empty()) {
                        makeDummyDirectory(entry, subpath, isLast);
                    }
                    else if (subpath.compare(entry->path) == 0) {
                        //TODO: does the exact child dir matter, since its just fake?
                        currentDirectory = entry->children[0];

                        if (isLast) {
                            makeDummyDirectory(entry, subpath, isLast);
                        }
                    }
                    else {
                        //TODO: error?
                    }

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

            if (index == 0) {
                //this result is empty
                break;
            }

            ptr += 4;
            auto type = static_cast<Cache::Type>(*ptr);
            ptr++;
            i += 5;
            auto path = reinterpret_cast<char*>(ptr);
            auto pathLength = strlen(path);

            Cache::Entry* entry;

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
                case Cache::Type::Union: {
                    auto unionDir = new Cache::Union();
                    entry = unionDir;
                    break;
                }
            }
            
            entry->cacheable = directory->cacheable;
            entry->needsRead = directory->cacheable;
            entry->writeable = directory->writeable;
            
            entry->type = static_cast<Cache::Type>(type);
            memcpy(entry->path, path, pathLength);
            entry->index = index;
            entry->pathLength = pathLength;

            directory->children.push_back(entry);

            ptr += pathLength + 1;
            i += pathLength + 1;
        }

        if (result.expectMore) {
            return;
        }

        directory->needsRead = false;
                
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

        auto subpaths = split(pending.remainingPath, '/', true);
        auto lastPath = subpaths.size();

        if (skipLast) {
            lastPath--;
        }

        for (auto i = 0u; i < lastPath; i++) {
            auto subpath = subpaths[i];
            bool isLast = i == (lastPath - 1);

            if (i > 0 && subpath.compare("/") == 0) {
                pending.remainingPath.remove_prefix(1);
                continue;
            }

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

                        if (isLast && subpath.compare(dir->path) == 0) {
                            pending.entry = dir;
                            pending.parent = dir;
                            return DiscoverResult::Succeeded;
                        }

                        bool canContinue {false};

                        for (auto child : dir->children) {
                            if (subpath.compare(child->path) == 0) {
                                if (isLast) {
                                    pending.entry = child;
                                    pending.parent = dir;

                                    if (child->type == Cache::Type::Directory
                                        && !child->cacheable) {
                                        return DiscoverResult::DependsOnMount;
                                    }

                                    if (child->type == Cache::Type::Directory) {
                                        pending.parent = static_cast<Cache::Directory*>(child);
                                        
                                        if (pending.parent->needsRead) {
                                            return DiscoverResult::DependsOnRead;
                                        }
                                    }

                                    return DiscoverResult::Succeeded;
                                }
                                else if (!child->cacheable) {
                                    pending.entry = child;

                                    if (child->type == Cache::Type::Directory) {
                                        pending.parent = static_cast<Cache::Directory*>(child);
                                    }

                                    pending.remainingPath.remove_prefix(subpath.length());
                                    return DiscoverResult::DependsOnMount;
                                }
                                else {
                                    currentDirectory = child;
                                    pending.parent = dir;
                                    pending.remainingPath.remove_prefix(subpath.length());
                                    canContinue = true;
                                    break;
                                }
                            }
                        }

                        //if we got here then there is no such file
                        if (!canContinue) {
                            return DiscoverResult::Failed;
                        }
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
                    auto unionDir = static_cast<Cache::Union*>(currentDirectory);

                    if (unionDir->children.empty() || subpath.compare(unionDir->path) != 0) {
                        return DiscoverResult::Failed;
                    }

                    auto subpending = pending;
                    subpending.remainingPath.remove_prefix(subpath.length());

                    for (auto childIndex = unionDir->children.size(); childIndex != 0; childIndex--) {

                        auto childDir = unionDir->children[childIndex - 1];
                        pending.breadcrumbs.push(childDir);
                    }

                    while (!pending.breadcrumbs.empty()) {
                        auto childDir = static_cast<Cache::Directory*>(pending.breadcrumbs.top());
                        pending.breadcrumbs.pop();
                        auto tempPending = subpending;

                        auto result = discoverPath(childDir, tempPending, skipLast);

                        if (result != DiscoverResult::Failed) {
                            pending = tempPending;
                            return result;
                        }

                    }

                    return DiscoverResult::Failed;
                }
            }
        } 

        return DiscoverResult::Failed;
    }

    void VirtualFileSystem::handleOpenRequest(OpenRequest& request) {
        std::string_view path {request.path, strlen(request.path)};
        Cache::Entry* currentDirectory = &root;

        PendingOpen pendingOpen;
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
            //pendingOpen.remainingPath = std::string_view{pendingOpen.fullPath + offset, pendingOpen.remainingPath.length()};
            pendingOpen.remainingPath = std::string_view{pendingOpen.fullPath, path.length()};
        };

        switch(result) {

            case DiscoverResult::Succeeded: {
                OpenRequest request;
                request.recipientId = pendingOpen.parent->mount;
                request.requestId = pendingRequest.id; 
                request.index = pendingOpen.entry->index;
                pendingOpen.remainingPath.copy(request.path, pendingOpen.remainingPath.length());

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
                pendingOpen.remainingPath.copy(request.path, pendingOpen.remainingPath.length());

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
            auto processVirtualFileDescriptor = findEmptyDescriptor(openFileDescriptors);
            if (processVirtualFileDescriptor >= 0) {                            
                openFileDescriptors[processVirtualFileDescriptor] = {
                    result.fileDescriptor,
                    pendingRequest.open.parent->mount,
                    pendingRequest.open.entry
                };
            }
            else {
                processVirtualFileDescriptor = openFileDescriptors.size();
                openFileDescriptors.push_back({
                    result.fileDescriptor,
                    pendingRequest.open.parent->mount,
                    pendingRequest.open.entry
                });
            }

            result.fileDescriptor = processVirtualFileDescriptor;
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
            //pendingCreate.remainingPath = std::string_view{pendingCreate.fullPath + offset, pendingCreate.remainingPath.length()};
            pendingCreate.remainingPath = std::string_view{pendingCreate.fullPath, path.length()};
        };

        if (pendingCreate.entry == nullptr || result == DiscoverResult::Failed) {
            printf("[VFS] tryCreate failed, pendingCreate.entry is null\n");
            return false;
        }

        if (pendingCreate.entry->type != Cache::Type::Directory) {
            return false;
        }

        auto parentDirectory = static_cast<Cache::Directory*>(pendingCreate.entry);

        switch(result) {

            case DiscoverResult::Succeeded: {
                CreateRequest request;
                request.recipientId = parentDirectory->mount;
                request.requestId = pendingRequest.id; 
                pendingCreate.remainingPath.copy(request.path, pendingCreate.remainingPath.length());

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
                pendingCreate.remainingPath.copy(request.path, pendingCreate.remainingPath.length());

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

        PendingOpen pendingCreate;
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
            auto& descriptor = openFileDescriptors[request.fileDescriptor];

            if (descriptor.isOpen()) {
                bool usedCache {false};

                if (descriptor.entry->cacheable) {
                    if (descriptor.entry->type == Cache::Type::Directory
                        && !descriptor.entry->needsRead) {
                        usedCache = true;
                        readDirectoryFromCache(request, descriptor);
                    }
                }

                if (!usedCache) {
                    request.requestId = nextRequestId++;
                    PendingRequest pending;
                    pending.type = RequestType::Read;
                    pending.id = request.requestId;
                    pending.requesterTaskId = request.senderTaskId;
                    pending.virtualFileDescriptor = request.fileDescriptor;
                    pendingRequests.push_back(pending);
                    request.recipientId = descriptor.mountTaskId;
                    request.fileDescriptor = descriptor.descriptor;
                    send(IPC::RecipientType::TaskId, &request);
                }
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

        if (pendingRequest == pendingRequests.end()) {
            printf("[VFS] Invalid pending request %d, handleReadResult\n", result.requestId);
            return;
        }

        result.recipientId = pendingRequest->requesterTaskId;
        //pendingRequests.erase(pendingRequest);
        send(IPC::RecipientType::TaskId, &result);
    }

    void VirtualFileSystem::handleRead512Result(Read512Result& result) {
        auto pendingRequest = getPendingRequest(result.requestId, pendingRequests);

        if (pendingRequest == pendingRequests.end()) {
            printf("[VFS] Invalid pending request %d, handleRead512Result\n", result.requestId);
            return;
        }

        openFileDescriptors[pendingRequest->virtualFileDescriptor].filePosition += result.bytesWritten;

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

        if (!result.expectReadResult) {
            pendingRequests.erase(pendingRequest);
        }

        send(IPC::RecipientType::TaskId, &result);
    }

    void VirtualFileSystem::handleCloseRequest(CloseRequest& request) {
        bool failed {false};

        auto sender = request.senderTaskId;

        if (request.fileDescriptor < openFileDescriptors.size()) {
            auto& descriptor = openFileDescriptors[request.fileDescriptor];

            if (descriptor.isOpen()) {
                request.fileDescriptor = descriptor.descriptor;
                request.recipientId = descriptor.mountTaskId;
                send(IPC::RecipientType::TaskId, &request);

                descriptor.close();

                CloseResult result;
                result.success = true;
                result.recipientId = sender;
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
            result.recipientId = sender;
            send(IPC::RecipientType::TaskId, &result);
        }
    }

    void VirtualFileSystem::readDirectoryFromCache(ReadRequest& request, VirtualFileDescriptor& descriptor) {
        ReadResult result;
        result.requestId = request.requestId;
        result.success = true;
        result.recipientId = request.senderTaskId;

        /*
        for directories, VirtualFileDescriptor::filePosition refers to the 
        current index of the children vector
        */
        auto directory = static_cast<Cache::Directory*>(descriptor.entry);
        auto numberOfChildren = directory->children.size();

        if (descriptor.filePosition < numberOfChildren) {
            uint32_t writeIndex = 0;
            uint32_t bytesRemaining = sizeof(result.buffer);
            
            for (auto i = descriptor.filePosition; i < numberOfChildren; i++) {
                auto entry = directory->children[i];

                if (bytesRemaining >= (5 + entry->pathLength + 1)) {
                    memcpy(result.buffer + writeIndex, &entry->index, sizeof(entry->index));
                    writeIndex += 4;
                    memcpy(result.buffer + writeIndex, &entry->type, sizeof(entry->type));
                    writeIndex += 1;
                    memcpy(result.buffer + writeIndex, entry->path, entry->pathLength);
                    writeIndex += entry->pathLength;
                    result.buffer[writeIndex] = '\0';
                    writeIndex++;
                    descriptor.filePosition++;
                }
                else {
                    break;
                }
            }

            result.bytesWritten = writeIndex;
        }

        send(IPC::RecipientType::TaskId, &result);
    }

    void VirtualFileSystem::messageLoop() {

        while (true) {
            IPC::MaximumMessageBuffer buffer;
            receive(&buffer);

            if (buffer.messageId == MountRequest::MessageId) {
                auto request = IPC::extractMessage<MountRequest>(buffer);
                handleMountRequest(request);
            }
            else if (buffer.messageId == GetDirectoryEntriesResult::MessageId) {
                auto result = IPC::extractMessage<GetDirectoryEntriesResult>(buffer);
                handleGetDirectoryEntriesResult(result);
            }
            else if (buffer.messageId == OpenRequest::MessageId) {
                auto request = IPC::extractMessage<OpenRequest>(buffer);
                handleOpenRequest(request);
            }
            else if (buffer.messageId == OpenResult::MessageId) {
                auto result = IPC::extractMessage<OpenResult>(buffer);
                handleOpenResult(result); 
            }
            else if (buffer.messageId == CreateRequest::MessageId) {
                auto request = IPC::extractMessage<CreateRequest>(buffer);
                handleCreateRequest(request);
            }
            else if (buffer.messageId == CreateResult::MessageId) {
                auto result = IPC::extractMessage<CreateResult>(buffer);
                handleCreateResult(result);
            }
            else if (buffer.messageId == ReadRequest::MessageId) {
                auto request = IPC::extractMessage<ReadRequest>(buffer);
                handleReadRequest(request);
                /*
                TODO: consider implementing either:
                1) message forwarding (could be insecure)
                2) a separate ReadRequestForwarded message
                */
            }
            else if (buffer.messageId == ReadResult::MessageId) {
                auto result = IPC::extractMessage<ReadResult>(buffer);
                handleReadResult(result);
            }
            else if (buffer.messageId == Read512Result::MessageId) {
                auto result = IPC::extractMessage<Read512Result>(buffer);
                handleRead512Result(result);
            }
            else if (buffer.messageId == WriteRequest::MessageId) {
                auto request = IPC::extractMessage<WriteRequest>(buffer);
                handleWriteRequest(request);
            }
            else if (buffer.messageId == WriteResult::MessageId) {
                auto result = IPC::extractMessage<WriteResult>(buffer);
                handleWriteResult(result);
            }
            else if (buffer.messageId == CloseRequest::MessageId) {
                auto request = IPC::extractMessage<CloseRequest>(buffer);
                handleCloseRequest(request);
            }
            else if (buffer.messageId == SeekRequest::MessageId) {
                printf("[VFS] Stub: SeekRequest\n");
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