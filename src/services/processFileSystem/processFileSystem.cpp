#include "processFileSystem.h"
#include <services.h>
#include <system_calls.h>
#include <string.h>
#include <services/virtualFileSystem/virtualFileSystem.h>
#include <stdio.h>
#include "object.h"

using namespace Kernel;
using namespace VFS;
using namespace Vostok;

namespace PFS {

    void registerMessages() {

    }

    enum class DescriptorType {
        Object,
        Function,
        Property
    };

    struct FileDescriptor {
        Object* instance;

        union {
            uint32_t functionId;
            uint32_t propertyId;
        };

        DescriptorType type;

        void read(uint32_t requesterTaskId) {
            if (type == DescriptorType::Object) {

            }
            else if (type == DescriptorType::Function) {
                instance->readFunction(requesterTaskId, functionId);
            }
            else if (type == DescriptorType::Property) {
                instance->readProperty(requesterTaskId, propertyId);
            }
        }

        void write(uint32_t requesterTaskId, ArgBuffer& args) {
            if (type == DescriptorType::Object) {

            }
            else if (type == DescriptorType::Function) {
                instance->writeFunction(requesterTaskId, functionId, args);
            }
            else if (type == DescriptorType::Property) {
                instance->writeProperty(requesterTaskId, propertyId, args);
            }
        }
    };

    void messageLoop() {

        uint32_t nextFileDescriptor {0};
        FileDescriptor openDescriptors[2];
        ProcessObject p;
        openDescriptors[0].instance = &p;
        openDescriptors[0].functionId = 0;

        while (true) {
            IPC::MaximumMessageBuffer buffer;
            receive(&buffer);

            if (buffer.messageId == OpenRequest::MessageId) {
                auto request = IPC::extractMessage<OpenRequest>(buffer);
                auto name = strrchr(request.path, '/');
                bool failed {true};
                OpenResult result{};
                result.serviceType = ServiceType::VFS;

                if (name != nullptr) {
                    //TODO: still hardcoded to one instance
                    auto functionId = p.getFunction(name + 1);

                    if (functionId >= 0) {
                        failed = false;
                        openDescriptors[0].functionId = functionId;
                        openDescriptors[0].type = DescriptorType::Function;
                        result.success = true;
                        result.fileDescriptor = nextFileDescriptor++;
                    }
                    else {
                        auto propertyId = p.getProperty(name + 1);

                        if (propertyId >= 0) {
                            failed = false;
                            openDescriptors[0].propertyId = propertyId;
                            openDescriptors[0].type = DescriptorType::Property;
                            result.success = true;
                            result.fileDescriptor = nextFileDescriptor++;
                        }
                    }
                }

                if (failed) {
                    result.success = false;
                }

                send(IPC::RecipientType::ServiceName, &result);
            }
            else if (buffer.messageId == ReadRequest::MessageId) {
                auto request = IPC::extractMessage<ReadRequest>(buffer);
                openDescriptors[0].read(request.senderTaskId);
            }
            else if (buffer.messageId == WriteRequest::MessageId) {
                auto request = IPC::extractMessage<WriteRequest>(buffer);
                ArgBuffer args{request.buffer, sizeof(request.buffer)};
                openDescriptors[0].write(request.senderTaskId, args);
            }
        }
    }

    void service() {

        waitForServiceRegistered(ServiceType::VFS);

        MountRequest request{};
        char* path = "/process";
        memcpy(request.path, path, strlen(path) + 1);
        request.serviceType = ServiceType::VFS;

        send(IPC::RecipientType::ServiceName, &request);

        messageLoop();
    }
}