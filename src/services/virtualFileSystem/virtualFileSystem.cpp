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
        uint32_t serviceId;
    };

    void messageLoop() {
        /*
        TODO: Replace with a trie where the leaf node values are services 
        */
        Mount mount;

        //TODO: design a proper data structure for holding outstanding requests
        uint32_t outstandingRequestSenderId {0};
        uint32_t openFileDescriptors[2];
        uint32_t nextFileDescriptor = 0;

        while (true) {
            IPC::MaximumMessageBuffer buffer;
            receive(&buffer);

            if (buffer.messageId == MountRequest::MessageId) {
                //TODO: for now only support top level mounts to /
                auto request = IPC::extractMessage<MountRequest>(buffer);
                mount.path = new char[strlen(request.path) + 1];
                memcpy(mount.path, request.path, strlen(request.path) + 1);
                mount.serviceId = request.serviceId; 
            }
            else if (buffer.messageId = OpenRequest::MessageId) {
                //TODO: search the trie for the appropriate mount point
                //since this is just an experiment to see if this design works, hardcode mount
                auto request = IPC::extractMessage<OpenRequest>(buffer);
                request.recipientId = mount.serviceId;
                send(IPC::RecipientType::TaskId, &request);   
                outstandingRequestSenderId = buffer.senderTaskId;
            }
            else if (buffer.messageId = OpenResult::MessageId) {
                if (buffer.senderTaskId == mount.serviceId) {
                    auto result = IPC::extractMessage<OpenResult>(buffer);

                    if (result.success) {
                        /*
                        mount services have their own local file descriptors,
                        VFS has its own that encompases all mounts
                        */
                        auto processFileDescriptor = nextFileDescriptor++;
                        openFileDescriptors[processFileDescriptor] = result.fileDescriptor;//mount.serviceId;
                        result.fileDescriptor = processFileDescriptor;
                        result.recipientId = outstandingRequestSenderId;
                        send(IPC::RecipientType::TaskId, &result);
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

                request.recipientId = mount.serviceId;
                request.fileDescriptor = openFileDescriptors[request.fileDescriptor];
                send(IPC::RecipientType::TaskId, &request);
            }
            else if (buffer.messageId == ReadResult::MessageId) {
                buffer.recipientId = outstandingRequestSenderId;
                send(IPC::RecipientType::TaskId, &buffer);
            }

        }
    }

    void service() {
        RegisterService registerRequest {};
        registerRequest.type = ServiceType::VFS;

        IPC::MaximumMessageBuffer buffer;

        send(IPC::RecipientType::ServiceRegistryMailbox, &registerRequest);
        receive(&buffer);

        if (buffer.messageId == GenericServiceMeta::MessageId) {
            registerMessages();
            messageLoop();
        }
    }
}