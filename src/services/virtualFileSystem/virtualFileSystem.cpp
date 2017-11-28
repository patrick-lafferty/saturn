#include "virtualFileSystem.h"
#include <services.h>
#include <system_calls.h>
#include <string.h>

using namespace Kernel;

namespace VFS {

    uint32_t MountRequest::MessageId;
    uint32_t OpenRequest::MessageId;
    uint32_t OpenResult::MessageId;
    uint32_t ReadRequest::MessageId;
    uint32_t ReadResult::MessageId;

    void registerMessages() {
        IPC::registerMessage<MountRequest>();
        IPC::registerMessage<OpenRequest>();
        IPC::registerMessage<OpenResult>();
        IPC::registerMessage<ReadRequest>();
        IPC::registerMessage<ReadResult>();
    }

    struct Mount {
        char* path;
        uint32_t pathLength;
        uint32_t serviceId;
    };

    struct FileDescriptor {
        uint32_t descriptor;
        uint32_t mount;
    };

    void messageLoop() {
        /*
        TODO: Replace with a trie where the leaf node values are services 
        */
        Mount mounts[2];

        //TODO: design a proper data structure for holding outstanding requests
        uint32_t outstandingRequestSenderId {0};
        FileDescriptor openFileDescriptors[10];
        uint32_t nextFileDescriptor = 0;
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

                for (auto& mount : mounts) {

                    if (strncmp(request.path, mount.path, mount.pathLength) == 0) {
                        request.recipientId = mount.serviceId;
                        break;
                    }
                }

                send(IPC::RecipientType::TaskId, &request);   
                outstandingRequestSenderId = buffer.senderTaskId;
            }
            else if (buffer.messageId == OpenResult::MessageId) {
                for (auto& mount : mounts) {

                    if (buffer.senderTaskId == mount.serviceId) {
                        auto result = IPC::extractMessage<OpenResult>(buffer);

                        if (result.success) {
                            /*
                            mount services have their own local file descriptors,
                            VFS has its own that encompases all mounts
                            */
                            auto processFileDescriptor = nextFileDescriptor++;
                            openFileDescriptors[processFileDescriptor] = {
                                result.fileDescriptor,
                                mount.serviceId
                            };

                            result.fileDescriptor = processFileDescriptor;
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
                auto descriptor = openFileDescriptors[request.fileDescriptor];

                request.recipientId = descriptor.mount;//mount.serviceId;
                request.fileDescriptor = descriptor.descriptor;//openFileDescriptors[request.fileDescriptor];
                send(IPC::RecipientType::TaskId, &request);
            }
            else if (buffer.messageId == ReadResult::MessageId) {
                auto result = IPC::extractMessage<ReadResult>(buffer);
                result.recipientId = outstandingRequestSenderId;
                send(IPC::RecipientType::TaskId, &result);
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