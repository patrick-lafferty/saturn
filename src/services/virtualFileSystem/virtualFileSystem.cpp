#include "virtualFileSystem.h"
#include <services.h>
#include <system_calls.h>
#include <string.h>
#include <vector>

using namespace Kernel;

namespace VFS {

    uint32_t MountRequest::MessageId;
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

    struct PendingRequest {
        uint32_t id;
        uint32_t requesterTaskId;
    };

    PendingRequest getPendingRequest(uint32_t requestId, std::vector<PendingRequest>& requests) {
        PendingRequest result;

        for (auto& request : requests) {
            if (request.id == requestId) {
                result = request;
                requests.erase(&request);
                break;
            }
        }

        return result;
    }

    void messageLoop() {
        /*
        TODO: Replace with a trie where the leaf node values are services 
        */
        std::vector<Mount> mounts;
        std::vector<FileDescriptor> openFileDescriptors;
        std::vector<PendingRequest> pendingRequests;
        uint32_t nextRequestId {0};

        while (true) {
            IPC::MaximumMessageBuffer buffer;
            receive(&buffer);

            if (buffer.messageId == MountRequest::MessageId) {
                //TODO: for now only support top level mounts to /
                auto request = IPC::extractMessage<MountRequest>(buffer);
                auto pathLength = strlen(request.path) + 1;
                mounts.push_back({nullptr, pathLength - 1, request.senderTaskId});
                auto& mount = mounts[mounts.size() - 1];
                mount.path = new char[pathLength];
                memcpy(mount.path, request.path, pathLength);

            }
            else if (buffer.messageId == OpenRequest::MessageId) {
                //TODO: search the trie for the appropriate mount point
                //since this is just an experiment to see if this design works, hardcode mount
                auto request = IPC::extractMessage<OpenRequest>(buffer);
                Mount mount;
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
                }
            }
            else if (buffer.messageId == OpenResult::MessageId) {

                for (auto& mount : mounts) {

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
                            auto processFileDescriptor = findEmptyDescriptor(openFileDescriptors);
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
                }
            }
            else if (buffer.messageId == CreateRequest::MessageId) {
                auto request = IPC::extractMessage<CreateRequest>(buffer);
                Mount mount;
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
                }
            }
            else if (buffer.messageId == CreateResult::MessageId) {
                auto result = IPC::extractMessage<CreateResult>(buffer);
                auto pendingRequest = getPendingRequest(result.requestId, pendingRequests);
                result.recipientId = pendingRequest.requesterTaskId;
                send(IPC::RecipientType::TaskId, &result);
            }
            else if (buffer.messageId == ReadRequest::MessageId) {
                auto request = IPC::extractMessage<ReadRequest>(buffer);

                /*
                TODO: consider implementing either:
                1) message forwarding (could be insecure)
                2) a separate ReadRequestForwarded message
                */
                bool failed {false};
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
                }
                
            }
            else if (buffer.messageId == ReadResult::MessageId) {
                auto result = IPC::extractMessage<ReadResult>(buffer);
                auto pendingRequest = getPendingRequest(result.requestId, pendingRequests);
                result.recipientId = pendingRequest.requesterTaskId;
                send(IPC::RecipientType::TaskId, &result);
            }
            else if (buffer.messageId == WriteRequest::MessageId) {
                auto request = IPC::extractMessage<WriteRequest>(buffer);

                bool failed {false};

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
                }
            }
            else if (buffer.messageId == WriteResult::MessageId) {
                auto result = IPC::extractMessage<WriteResult>(buffer);
                auto pendingRequest = getPendingRequest(result.requestId, pendingRequests);
                result.recipientId = pendingRequest.requesterTaskId;
                send(IPC::RecipientType::TaskId, &result);
            }
            else if (buffer.messageId == CloseRequest::MessageId) {
                auto request = IPC::extractMessage<CloseRequest>(buffer);

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
                }
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
        messageLoop();
    }
}