#include "virtualFileSystem.h"
#include <services.h>
#include <system_calls.h>
#include <string.h>
#include <array.h>

using namespace Kernel;

namespace VFS {

    uint32_t MountRequest::MessageId;
    uint32_t OpenRequest::MessageId;
    uint32_t OpenResult::MessageId;
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

    int findEmptyDescriptor(Array<FileDescriptor>& descriptors) {
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

    void messageLoop() {
        /*
        TODO: Replace with a trie where the leaf node values are services 
        */
        Mount mounts[2] = {
            {nullptr, 0, 0},
            {nullptr, 0, 0}
        };

        //TODO: design a proper data structure for holding outstanding requests
        uint32_t outstandingRequestSenderId {0};
        Array<FileDescriptor> openFileDescriptors;
        uint32_t nextMount {0};

        while (true) {
            IPC::MaximumMessageBuffer buffer;
            receive(&buffer);

            if (buffer.messageId == MountRequest::MessageId) {
                //TODO: for now only support top level mounts to /
                auto request = IPC::extractMessage<MountRequest>(buffer);
                auto pathLength = strlen(request.path) + 1;
                mounts[nextMount].path = new char[pathLength];
                memcpy(mounts[nextMount].path, request.path, pathLength);
                mounts[nextMount].serviceId = request.senderTaskId; 
                mounts[nextMount].pathLength = pathLength;
                nextMount++;
            }
            else if (buffer.messageId == OpenRequest::MessageId) {
                //TODO: search the trie for the appropriate mount point
                //since this is just an experiment to see if this design works, hardcode mount
                auto request = IPC::extractMessage<OpenRequest>(buffer);
                bool foundMount {false};

                for (auto& mount : mounts) {

                    if (!mount.exists()) {
                        continue;
                    }

                    if (strncmp(request.path, mount.path, mount.pathLength) == 0) {
                        request.recipientId = mount.serviceId;
                        foundMount = true;
                        break;
                    }
                }

                if (foundMount) {
                    send(IPC::RecipientType::TaskId, &request);   
                    outstandingRequestSenderId = buffer.senderTaskId;
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
                                openFileDescriptors.add({
                                    result.fileDescriptor,
                                    mount.serviceId
                                });
                            }

                            result.fileDescriptor = processFileDescriptor;
                            result.recipientId = outstandingRequestSenderId;
                            send(IPC::RecipientType::TaskId, &result);
                        }
                        else {
                            result.recipientId = outstandingRequestSenderId;
                            send(IPC::RecipientType::TaskId, &result);
                        }

                    }
                }
            }
            else if (buffer.messageId == ReadRequest::MessageId) {
                auto request = IPC::extractMessage<ReadRequest>(buffer);
                outstandingRequestSenderId = request.senderTaskId;

                /*
                TODO: consider implementing either:
                1) message forwarding (could be insecure)
                2) a separate ReadRequestForwarded message
                */
                bool failed {false};
                if (request.fileDescriptor < openFileDescriptors.size()) {
                    auto descriptor = openFileDescriptors[request.fileDescriptor];

                    if (descriptor.isOpen()) {
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
                result.recipientId = outstandingRequestSenderId;
                send(IPC::RecipientType::TaskId, &result);
            }
            else if (buffer.messageId == WriteRequest::MessageId) {
                auto request = IPC::extractMessage<WriteRequest>(buffer);
                outstandingRequestSenderId = request.senderTaskId;

                bool failed {false};

                if (request.fileDescriptor < openFileDescriptors.size()) {
                    auto descriptor = openFileDescriptors[request.fileDescriptor];

                    if (descriptor.isOpen()) {
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
                result.recipientId = outstandingRequestSenderId;
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

                        OpenResult result;
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
                    OpenResult result;
                    result.success = false;
                    result.recipientId = request.senderTaskId;
                    send(IPC::RecipientType::TaskId, &result);
                }
            }

        }
    }

    void registerService() {
        RegisterService registerRequest {};
        registerRequest.type = ServiceType::VFS;

        IPC::MaximumMessageBuffer buffer;

        send(IPC::RecipientType::ServiceRegistryMailbox, &registerRequest);
        receive(&buffer);

        if (buffer.messageId == GenericServiceMeta::MessageId) {
            registerMessages();
        }
    }

    void service() {
        registerService(); 
        messageLoop();
    }
}