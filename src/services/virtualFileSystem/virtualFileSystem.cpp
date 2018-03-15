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
#include "virtualFileSystem.h"
#include <services.h>
#include <system_calls.h>
#include <string.h>
#include <vector>
#include <saturn/parsing.h>

using namespace Kernel;

namespace VirtualFileSystem {

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
        //nextId = new uint32_t;
        nextId = 0;
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
        
        return std::find_if(begin(requests), end(requests), [&](const auto& a) {
            return a.id == requestId;
        });
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

        for (auto& observer : mountObservers) {
            if (path.compare(observer.path) == 0) {
                MountNotification notification;
                path.copy(notification.path, sizeof(notification.path));

                for (auto taskId : observer.subscribers) {
                    notification.recipientId = taskId;
                    send(IPC::RecipientType::TaskId, &notification);
                }

                //TODO: erase observer

                break;
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

        auto requestId = getNextRequestId();    
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
        //auto pathStart = pendingOpen.remainingPath.data();
        auto result = discoverPath(currentDirectory, pendingOpen);
        auto requestId = pendingRequest.id;
        
        auto savePath = [&]() {
            if (pendingOpen.fullPath != nullptr) {
                return;
            }

            auto path = pendingOpen.remainingPath;

            pendingOpen.fullPath = new char[path.length()];
            path.copy(pendingOpen.fullPath, path.length());
            //auto offset = pendingOpen.remainingPath.data() - pathStart;
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

        if (request == pendingRequests.end()) {
            printf("[VFS] Invalid pending request %d, handleOpenResult\n", result.requestId);
            return;
        }

        if (request->type != RequestType::Open) {
            printf("[VFS] Wrong request type, handleOpenResult\n");
            return;
        }

        auto& pendingRequest = *request;

        if (result.success) {
            auto processVirtualFileDescriptor = findEmptyDescriptor(openFileDescriptors);

            if (processVirtualFileDescriptor >= 0) {                            
                openFileDescriptors[processVirtualFileDescriptor] = {
                    result.fileDescriptor,
                    pendingRequest.open.parent->mount,
                    pendingRequest.open.entry,
                    0
                };
            }
            else {
                processVirtualFileDescriptor = openFileDescriptors.size();
                openFileDescriptors.push_back({
                    result.fileDescriptor,
                    pendingRequest.open.parent->mount,
                    pendingRequest.open.entry,
                    0
                });
            }

            if (pendingRequest.open.entry->type == Cache::Type::File) {
                auto file = static_cast<Cache::File*>(pendingRequest.open.entry);

                if (file->data.empty()) {
                    file->data.resize(1 + result.fileLength / 512);
                }
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
        //auto pathStart = pendingCreate.remainingPath.data();
        auto result = discoverPath(currentDirectory, pendingCreate);
        auto requestId = pendingRequest.id;
        
        auto savePath = [&]() {
            if (pendingCreate.fullPath != nullptr) {
                return;
            }

            auto path = pendingCreate.remainingPath;

            pendingCreate.fullPath = new char[path.length()];
            path.copy(pendingCreate.fullPath, path.length());
            //TODO: is offset used anymore?
            //auto offset = pendingCreate.remainingPath.data() - pathStart;
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

        auto requestId = getNextRequestId();    
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

        if (request->type != RequestType::Create) {
            printf("[VFS] Wrong request type, handleCreateResult\n");
            return;
        }

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

    uint32_t VirtualFileSystem::getNextRequestId() {
        return nextId++;
    }

    void VirtualFileSystem::handleReadRequest(ReadRequest& request) {

        bool failed {false};

        if (request.fileDescriptor < openFileDescriptors.size()) {
            auto& descriptor = openFileDescriptors[request.fileDescriptor];

            if (descriptor.isOpen()) {
                bool usedCache {false};
                bool needsSync {false};
                bool isPreparingCache {false};

                int remainingBlocks = 0;

                if (descriptor.entry->cacheable) {
                    if (descriptor.entry->type == Cache::Type::Directory
                        && !descriptor.entry->needsRead) {
                        usedCache = true;
                        readDirectoryFromCache(request, descriptor);
                    }
                    else if (descriptor.entry->type == Cache::Type::File) {
                        auto file = static_cast<Cache::File*>(descriptor.entry);
                        auto currentBlock = descriptor.filePosition / 512;
                        auto endBlock = (descriptor.filePosition + request.readLength) / 512;
                        auto currentByte = descriptor.filePosition % 512;
                        auto remainingBytesInBlock = std::min(512 - currentByte, request.readLength);

                        if (file->data[currentBlock].hasValue) {
                            Read512Result result;
                            result.success = true;
                            memcpy(result.buffer, file->data[currentBlock].data + currentByte, remainingBytesInBlock);
                            result.bytesWritten = remainingBytesInBlock;
                            result.expectMore = false;
                            result.recipientId = request.senderTaskId;
                            descriptor.filePosition += result.bytesWritten;

                            if (endBlock != currentBlock) {
                                if (file->data[endBlock].hasValue) {
                                    auto remaining = request.readLength - remainingBytesInBlock;
                                    memcpy(result.buffer + remainingBytesInBlock, 
                                        file->data[endBlock].data,
                                        remaining);
                                    
                                    result.bytesWritten += remaining;
                                    descriptor.filePosition += remaining;
                                    usedCache = true;
                                }
                                else {
                                    result.expectMore = true;
                                    request.readLength -= result.bytesWritten;
                                    remainingBlocks = 1;
                                    isPreparingCache = true;
                                }

                            }
                            else {
                                usedCache = true;
                            }

                            needsSync = true;

                            send(IPC::RecipientType::TaskId, &result);

                        }
                        else {
                            remainingBlocks = 1;
                            isPreparingCache = true;
                            needsSync = true;

                            if (endBlock != currentBlock && (request.readLength - remainingBytesInBlock) > 0) {
                                remainingBlocks = 2;
                            }
                        }
                        
                    }
                }

                if (needsSync) {
                    SyncPositionWithCache sync;
                    sync.fileDescriptor = descriptor.descriptor;
                    sync.recipientId = descriptor.mountTaskId;
                    
                    if (isPreparingCache) {
                        sync.filePosition = 512 * (descriptor.filePosition / 512);
                    }
                    else {
                        sync.filePosition = descriptor.filePosition;
                    }

                    send(IPC::RecipientType::TaskId, &sync);
                }

                if (!usedCache) {
                    auto requestId = getNextRequestId();
                    request.requestId = requestId;

                    PendingRequest pending;
                    pending.type = RequestType::Read;
                    pending.id = requestId;
                    pending.requesterTaskId = request.senderTaskId;
                    pending.virtualFileDescriptor = request.fileDescriptor;
                    pending.read.remainingBlocks = remainingBlocks;
                    pending.read.currentBlock = 0;
                    pending.read.blocks[0].index = descriptor.filePosition / 512;

                    if (remainingBlocks > 1) {
                        pending.read.blocks[1].index = pending.read.blocks[0].index + 1;
                    }

                    pending.read.filePosition = descriptor.filePosition;
                    pending.read.readLength = request.readLength;
                    pendingRequests.push_back(pending);

                    request.recipientId = descriptor.mountTaskId;
                    request.fileDescriptor = descriptor.descriptor;

                    if (isPreparingCache) {
                        request.readLength = 512;
                    }

                    send(IPC::RecipientType::TaskId, &request);

                    if (remainingBlocks > 1) {
                        send(IPC::RecipientType::TaskId, &request);
                    }
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

        if (pendingRequest->type != RequestType::Read) {
            printf("[VFS] Wrong request type, handleReadResult\n");
            return;
        }

        result.recipientId = pendingRequest->requesterTaskId;
        pendingRequests.erase(pendingRequest);
        send(IPC::RecipientType::TaskId, &result);
    }

    void VirtualFileSystem::handleRead512Result(Read512Result& result) {
        auto pendingRequest = getPendingRequest(result.requestId, pendingRequests);

        if (pendingRequest == pendingRequests.end()) {
            printf("[VFS] Invalid pending request %d, handleRead512Result\n", result.requestId);
            return;
        }

        if (pendingRequest->type != RequestType::Read
            && pendingRequest->type != RequestType::Stream) {
            printf("[VFS] Wrong request type, handleRead512Result\n");
            return;
        }
        
        if (pendingRequest->virtualFileDescriptor >= openFileDescriptors.size()) {
            printf("[VFS] Missing openFileDescriptor, read512Result\n");
        }

        auto& descriptor = openFileDescriptors[pendingRequest->virtualFileDescriptor];
        bool canSend {true};

        if (descriptor.entry->type == Cache::Type::File) {

            if (pendingRequest->type == RequestType::Read) {

                if (pendingRequest->read.currentBlock < pendingRequest->read.remainingBlocks) {

                    auto file = static_cast<Cache::File*>(descriptor.entry);
                    auto block = pendingRequest->read.blocks[pendingRequest->read.currentBlock].index;

                    memcpy(file->data[block].data,
                        result.buffer, 
                        512);

                    if (file->data[block].hasValue) {
                        //printf("[VFS] Updating file block cache\n");
                    }

                    file->data[block].hasValue = true;

                    pendingRequest->read.currentBlock++;

                    if (pendingRequest->read.remainingBlocks == pendingRequest->read.currentBlock) {
                        auto startingBlock = pendingRequest->read.filePosition / 512;
                        auto endingBlock = (pendingRequest->read.filePosition + pendingRequest->read.readLength) / 512;
                        auto startingByte = pendingRequest->read.filePosition % 512;

                        memset(result.buffer, 0, 512);
                        auto bytesToEnd = std::min(512 - startingByte, pendingRequest->read.readLength);

                        memcpy(result.buffer, 
                            file->data[startingBlock].data + startingByte,
                            bytesToEnd);

                        if (endingBlock != startingBlock && (pendingRequest->read.readLength - bytesToEnd) > 0) {
                            memcpy(result.buffer + bytesToEnd,
                                file->data[endingBlock].data,
                                pendingRequest->read.readLength - bytesToEnd);
                        }

                        result.bytesWritten = pendingRequest->read.readLength;
                        result.expectMore = false;
                        canSend = true;

                        SyncPositionWithCache sync;
                        sync.fileDescriptor = descriptor.descriptor;
                        sync.recipientId = descriptor.mountTaskId;
                        sync.filePosition = pendingRequest->read.filePosition + pendingRequest->read.readLength;
                        descriptor.filePosition = sync.filePosition;
                        send(IPC::RecipientType::TaskId, &sync);
                    }
                    else {
                        canSend = false;
                    }
                }
            }
            else {
                canSend = false;

                auto file = static_cast<Cache::File*>(descriptor.entry);
                auto block = pendingRequest->stream.blocks[pendingRequest->stream.currentBlock].index;

                memcpy(file->data[block].data,
                    result.buffer, 
                    512);

                file->data[block].hasValue = true;

                pendingRequest->stream.currentBlock++;
                pendingRequest->stream.remainingBlocks--;

                if (pendingRequest->stream.remainingBlocks == 0
                    && pendingRequest->stream.bufferShareWasSuccessful) {
                    completeReadStreamRequest(*pendingRequest, descriptor);
                    pendingRequests.erase(pendingRequest);
                }
            }
        }
        else {
            descriptor.filePosition += result.bytesWritten;
        }

        result.recipientId = pendingRequest->requesterTaskId;
        
        if (canSend && !result.expectMore) {
            pendingRequests.erase(pendingRequest);
        }

        if (canSend) {
            send(IPC::RecipientType::TaskId, &result);
        }
    }

    void VirtualFileSystem::handleReadStreamRequest(ReadStreamRequest& request) {

        bool failed {true};

        if (request.fileDescriptor < openFileDescriptors.size()) {
            auto& descriptor = openFileDescriptors[request.fileDescriptor];

            if (descriptor.isOpen()) {
                if (descriptor.entry->cacheable) {
                    if (descriptor.entry->type == Cache::Type::File) {

                        auto file = static_cast<Cache::File*>(descriptor.entry);
                        auto currentBlock = descriptor.filePosition / 512;
                        auto endBlock = (descriptor.filePosition + request.readLength) / 512;
                        auto currentByte = descriptor.filePosition % 512;
                        auto remainingBytesInBlock = std::min(512 - currentByte, request.readLength);
                        auto blocksToRead = 0;

                        PendingRequest pending;
                        pending.id = getNextRequestId();
                        pending.requesterTaskId = request.senderTaskId;
                        pending.type = RequestType::Stream;
                        pending.virtualFileDescriptor = request.fileDescriptor;

                        auto currentFilePosition = currentBlock * 512;

                        /*
                        Check if the file's cache has all of the required blocks,
                        and send read requests to populate the cache if necessary.
                        */
                        for (auto block = currentBlock; block <= endBlock; block++) {
                            if (!file->data[block].hasValue) {
                                blocksToRead++;
                                pending.stream.blocks.push_back({block, false});
                            }
                        }

                        pending.stream.startingFilePosition = descriptor.filePosition;
                        pending.stream.readLength = request.readLength;
                        pending.stream.remainingBlocks = blocksToRead;
                        pendingRequests.push_back(pending);

                        for (auto block = currentBlock; block <= endBlock; block++) {
                            if (!file->data[block].hasValue) {

                                ReadRequest read;
                                read.requestId = pending.id;
                                read.fileDescriptor = descriptor.descriptor;
                                read.readLength = 512;
                                read.recipientId = descriptor.mountTaskId;
                                read.filePosition = block * 512;
                                send(IPC::RecipientType::TaskId, &read);
                            }
                        }

                        failed = false;
                    }
                }
                else {
                    //TODO: handle non-cacheable things
                }
            }
        }

        if (failed) {
            ReadStreamResult result;
            result.success = false;
            result.recipientId = request.senderTaskId;
            send(IPC::RecipientType::TaskId, &result);
        }
    }

    void VirtualFileSystem::handleWriteRequest(WriteRequest& request) {
        bool failed {false};

        if (request.fileDescriptor < openFileDescriptors.size()) {
            auto descriptor = openFileDescriptors[request.fileDescriptor];

            if (descriptor.isOpen()) {
                request.requestId = getNextRequestId();
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
        else {
            pendingRequest->type = RequestType::Read;
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

    void VirtualFileSystem::handleSeekRequest(SeekRequest& request) {
        bool failed {false};
        if (request.fileDescriptor < openFileDescriptors.size()) {
            auto& descriptor = openFileDescriptors[request.fileDescriptor];

            if (descriptor.isOpen()) {
                request.requestId = getNextRequestId();
                PendingRequest pending;
                pending.type = RequestType::Seek;
                pending.id = request.requestId;
                pending.requesterTaskId = request.senderTaskId;
                pending.virtualFileDescriptor = request.fileDescriptor;
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
            SeekResult result;
            result.success = false;
            result.recipientId = request.senderTaskId;
            send(IPC::RecipientType::TaskId, &result);
        }
    }

    void VirtualFileSystem::handleSeekResult(SeekResult& result) {
        auto pendingRequest = getPendingRequest(result.requestId, pendingRequests);
        result.recipientId = pendingRequest->requesterTaskId;

        if (pendingRequest->virtualFileDescriptor< openFileDescriptors.size()) {

            auto& descriptor = openFileDescriptors[pendingRequest->virtualFileDescriptor]; 
            if (descriptor.isOpen()) {
                descriptor.filePosition = result.filePosition;
            }
        }

        pendingRequests.erase(pendingRequest);

        send(IPC::RecipientType::TaskId, &result);
    }

    void VirtualFileSystem::handleSubscribeMount(SubscribeMount& request) {
        PendingOpen pending;
        pending.remainingPath = {request.path, strlen(request.path)};

        if (discoverPath(&root, pending) != DiscoverResult::Failed) {
            MountNotification notification;
            strncpy(notification.path, request.path, sizeof(notification.path));
            notification.recipientId = request.senderTaskId;
            send(IPC::RecipientType::TaskId, &notification);
        }
        else {

            for (auto& observer : mountObservers) {
                if (strncmp(request.path, observer.path, 64) == 0) {
                    observer.subscribers.push_back(request.senderTaskId);
                    return;
                }
            }

            MountObserver observer;
            strncpy(observer.path, request.path, 64);
            observer.subscribers.push_back(request.senderTaskId);
            mountObservers.push_back(observer);
        }
    }

    void VirtualFileSystem::handleShareMemoryInvitation(Kernel::ShareMemoryInvitation& invitation) {
        auto request = std::find_if(begin(pendingRequests), end(pendingRequests), [&](const auto& a) {
            return a.requesterTaskId == invitation.senderTaskId
                && a.type == RequestType::Stream;
        });

        Kernel::ShareMemoryResponse response;
        response.recipientId = invitation.senderTaskId;

        if (request != end(pendingRequests)) {
            response.accepted = true;
            auto space = invitation.size;
            auto buffer = (uint8_t*)aligned_alloc(0x1000, space);
            request->stream.buffer = buffer;
            response.sharedAddress = reinterpret_cast<uintptr_t>(buffer);
        }
        else {
            response.accepted = false;
        }

        send(IPC::RecipientType::TaskId, &response);
    }

    void completeReadStreamRequest(PendingRequest& request, VirtualFileDescriptor& descriptor) {
        auto file = static_cast<Cache::File*>(descriptor.entry);

        auto startingBlock = request.stream.startingFilePosition / 512;
        auto endingBlock = (request.stream.startingFilePosition + request.stream.readLength) / 512;
        auto startingByte = request.stream.startingFilePosition % 512;
        auto bufferOffset = request.stream.pageOffset;
        auto remainingBytes = request.stream.readLength;
        auto buffer = request.stream.buffer;

        for (auto block = startingBlock; block <= endingBlock; block++) {
            uint32_t copiedBytes {0};

            if (block == startingBlock && startingByte > 0) {
                copiedBytes = std::min(remainingBytes, 512 - startingByte);

                memcpy(buffer + bufferOffset, 
                    file->data[block].data + startingByte,
                    copiedBytes
                );
            }
            else {
                copiedBytes = std::min(remainingBytes, 512u);

                memcpy(buffer + bufferOffset, 
                    file->data[block].data,
                    copiedBytes
                );
            }

            remainingBytes -= copiedBytes;
            bufferOffset += copiedBytes;
        }

        ReadStreamResult streamResult;
        streamResult.success = true;
        streamResult.bytesWritten = request.stream.readLength;
        streamResult.recipientId = request.requesterTaskId;

        send(IPC::RecipientType::TaskId, &streamResult);
        //free request.stream.buffer;


        SyncPositionWithCache sync;
        sync.fileDescriptor = descriptor.descriptor;
        sync.recipientId = descriptor.mountTaskId;
        sync.filePosition = request.stream.startingFilePosition + request.stream.readLength;
        descriptor.filePosition = sync.filePosition;
        send(IPC::RecipientType::TaskId, &sync);
    }

    void VirtualFileSystem::handleShareMemoryResult(Kernel::ShareMemoryResult& result) {

        auto request = std::find_if(begin(pendingRequests), end(pendingRequests), [&](const auto& a) {
            return a.requesterTaskId == result.senderTaskId
                && a.type == RequestType::Stream;
        });

        if (request != end(pendingRequests)) {
            if (result.succeeded) {

                request->stream.bufferShareWasSuccessful = true;
                request->stream.pageOffset = result.pageOffset;

                if (request->stream.remainingBlocks == 0) {

                    if (request->virtualFileDescriptor < openFileDescriptors.size()) {
                        auto& descriptor = openFileDescriptors[request->virtualFileDescriptor];

                        completeReadStreamRequest(*request, descriptor);
                        pendingRequests.erase(request);
                    }
                }
            }
            else {

                ReadStreamResult streamResult;
                streamResult.success = false;
                streamResult.bytesWritten = 0;
                streamResult.recipientId = request->requesterTaskId;

                send(IPC::RecipientType::TaskId, &streamResult);

                delete request->stream.buffer;

                pendingRequests.erase(request);
            }
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

            switch(buffer.messageNamespace) {
                case IPC::MessageNamespace::VFS: {

                    switch(static_cast<MessageId>(buffer.messageId)) {
                        case MessageId::MountRequest: {
                            auto request = IPC::extractMessage<MountRequest>(buffer);
                            handleMountRequest(request);
                            break;
                        }
                        case MessageId::GetDirectoryEntriesResult: {
                            auto result = IPC::extractMessage<GetDirectoryEntriesResult>(buffer);
                            handleGetDirectoryEntriesResult(result);
                            break;
                        }
                        case MessageId::OpenRequest: {
                            auto request = IPC::extractMessage<OpenRequest>(buffer);
                            handleOpenRequest(request);
                            break;
                        }
                        case MessageId::OpenResult: {
                            auto result = IPC::extractMessage<OpenResult>(buffer);
                            handleOpenResult(result);
                            break;
                        }
                        case MessageId::CreateRequest: {
                            auto request = IPC::extractMessage<CreateRequest>(buffer);
                            handleCreateRequest(request);
                            break;
                        }
                        case MessageId::CreateResult: {
                            auto result = IPC::extractMessage<CreateResult>(buffer);
                            handleCreateResult(result);
                            break;
                        }
                        case MessageId::ReadRequest: {
                            auto request = IPC::extractMessage<ReadRequest>(buffer);
                            handleReadRequest(request);
                            /*
                            TODO: consider implementing either:
                            1) message forwarding (could be insecure)
                            2) a separate ReadRequestForwarded message
                            */
                            break;
                        }
                        case MessageId::ReadResult: {
                            auto result = IPC::extractMessage<ReadResult>(buffer);
                            handleReadResult(result);
                            break;
                        }
                        case MessageId::Read512Result: {
                            auto result = IPC::extractMessage<Read512Result>(buffer);
                            handleRead512Result(result);
                            break;
                        }
                        case MessageId::ReadStreamRequest: {
                            auto request = IPC::extractMessage<ReadStreamRequest>(buffer);
                            handleReadStreamRequest(request);
                            break;
                        }
                        case MessageId::WriteRequest: {
                            auto request = IPC::extractMessage<WriteRequest>(buffer);
                            handleWriteRequest(request);
                            break;
                        }
                        case MessageId::WriteResult: {
                            auto result = IPC::extractMessage<WriteResult>(buffer);
                            handleWriteResult(result);
                            break;
                        }
                        case MessageId::CloseRequest: {
                            auto request = IPC::extractMessage<CloseRequest>(buffer);
                            handleCloseRequest(request);
                            break;
                        }
                        case MessageId::SeekRequest: {
                            auto request = IPC::extractMessage<SeekRequest>(buffer);
                            handleSeekRequest(request);
                            break;
                        }
                        case MessageId::SeekResult: {
                            auto result = IPC::extractMessage<SeekResult>(buffer);
                            handleSeekResult(result);
                            break;
                        }
                        case MessageId::SubscribeMount: {
                            auto request = IPC::extractMessage<SubscribeMount>(buffer);
                            handleSubscribeMount(request);
                            break;
                        }
                        default: {
                            printf("[VFS] Unhandled message id\n");
                        }
                    }

                    break;
                }
                case IPC::MessageNamespace::ServiceRegistry: {
                    switch(static_cast<Kernel::MessageId>(buffer.messageId)) {
                        case Kernel::MessageId::ShareMemoryInvitation: {
                            auto message = IPC::extractMessage<Kernel::ShareMemoryInvitation>(buffer);
                            handleShareMemoryInvitation(message);
                            break;
                        }
                        case Kernel::MessageId::ShareMemoryResult: {
                            auto message = IPC::extractMessage<Kernel::ShareMemoryResult>(buffer);
                            handleShareMemoryResult(message);
                            break;
                        }
                        default:
                            break;
                    }

                    break;
                }
                default:
                    break;
            }

        }
    }

    void registerService() {
        RegisterService registerRequest;
        registerRequest.type = ServiceType::VFS;

        IPC::MaximumMessageBuffer buffer;

        send(IPC::RecipientType::ServiceRegistryMailbox, &registerRequest);
        receive(&buffer);

        NotifyServiceReady ready;
        send(IPC::RecipientType::ServiceRegistryMailbox, &ready);
    }

    void service() {
        registerService(); 

        auto system = new VirtualFileSystem();
        system->messageLoop();
    }
}