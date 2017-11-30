#include "processFileSystem.h"
#include <services.h>
#include <system_calls.h>
#include <string.h>
#include <services/virtualFileSystem/virtualFileSystem.h>
#include <stdio.h>
#include "object.h"
#include <stdlib.h>

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
        PFS::ProcessObject* instance;

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

    bool findObject(Array<ProcessObject>& objects, uint32_t pid, ProcessObject** found) {
        for (auto& object : objects) {
            if (object.pid == pid) {
                *found = &object;
                return true;
            }
        }

        return false;
    }

    void handleOpenRequest(OpenRequest& request, Array<ProcessObject>& processes, Array<FileDescriptor>& openDescriptors) {
        bool failed {true};
        OpenResult result{};
        result.serviceType = ServiceType::VFS;

        auto word = markWord(request.path, '/');
        auto start = word + 1;
        word = markWord(start, '/');

        if (strcmp(start, "process") == 0) {
            start = word + 1;
            word = markWord(start, '/');

            if (word != nullptr) {
                uint32_t pid = strtol(start, nullptr, 10);
                ProcessObject* process;
                auto found = findObject(processes, pid, &process);

                if (found) {
                    start = word + 1;
                    word = markWord(start, '/');

                    if (word != nullptr) {
                        auto functionId = process->getFunction(start);

                        if (functionId >= 0) {
                            failed = false;
                            /*openDescriptors[0].functionId = functionId;
                            openDescriptors[0].type = DescriptorType::Function;*/
                            result.success = true;
                            //result.fileDescriptor = nextFileDescriptor++;
                            openDescriptors.add({process, static_cast<uint32_t>(functionId), DescriptorType::Function});
                            result.fileDescriptor = openDescriptors.size() - 1;
                        }
                        else {
                            auto propertyId = process->getProperty(start);

                            if (propertyId >= 0) {
                                failed = false;
                                /*openDescriptors[0].propertyId = propertyId;
                                openDescriptors[0].type = DescriptorType::Property;*/
                                result.success = true;
                                //result.fileDescriptor = nextFileDescriptor++;
                                openDescriptors.add({process, static_cast<uint32_t>(propertyId), DescriptorType::Property});
                                result.fileDescriptor = openDescriptors.size() - 1;
                            }
                            
                        }
                    }
                }
            }
            else {
                failed = false;
                result.success = true;
                openDescriptors.add({nullptr, 0, DescriptorType::Object});
                result.fileDescriptor = openDescriptors.size() - 1;
            }
        }

        if (failed) {
            result.success = false;
        }

        send(IPC::RecipientType::ServiceName, &result);
    }

    void handleCreateRequest(CreateRequest& request, Array<ProcessObject>& processes, Array<FileDescriptor>& openDescriptors) {
        bool failed {true};
        CreateResult result{};
        result.serviceType = ServiceType::VFS;

        auto word = markWord(request.path, '/');
        auto start = word + 1;
        word = markWord(start, '/');

        if (strcmp(start, "process") == 0) {
            start = word + 1;
            word = markWord(start, '/');

            if (word != nullptr) {
                uint32_t pid = strtol(start, nullptr, 10);
                ProcessObject* process;
                auto found = findObject(processes, pid, &process);

                if (!found) {
                    ProcessObject p {pid};
                    processes.add(p);
                    failed = false;
                    result.success = true;
                }
            }
        }

        if (failed) {
            result.success = false;
        }

        send(IPC::RecipientType::ServiceName, &result);
    } 

    void handleReadRequest(ReadRequest& request, Array<ProcessObject>& processes, Array<FileDescriptor>& openDescriptors) {
        auto& descriptor = openDescriptors[request.fileDescriptor];

        if (descriptor.type == DescriptorType::Object) {
            ReadResult result;
            result.success = true;
            ArgBuffer args{result.buffer, sizeof(result.buffer)};
            args.writeType(ArgTypes::Property);

            for (auto& desc : openDescriptors) {
                if (desc.instance != nullptr) 
                args.writeValueWithType(desc.instance->pid, ArgTypes::Uint32);
            }

            args.writeType(ArgTypes::EndArg);

            result.recipientId = request.senderTaskId;
            send(IPC::RecipientType::TaskId, &result);
        }
        else {
            descriptor.read(request.senderTaskId);
        }
    }

    void handleWriteRequest(WriteRequest& request, Array<ProcessObject>& processes, Array<FileDescriptor>& openDescriptors) {
        ArgBuffer args{request.buffer, sizeof(request.buffer)};
        openDescriptors[request.fileDescriptor].write(request.senderTaskId, args);
    }

    void messageLoop() {

        /*uint32_t nextFileDescriptor {0};
        FileDescriptor openDescriptors[2];
        ProcessObject p;
        openDescriptors[0].instance = &p;
        openDescriptors[0].functionId = 0;*/
        Array<ProcessObject> processes;
        Array<FileDescriptor> openDescriptors;

        while (true) {
            IPC::MaximumMessageBuffer buffer;
            receive(&buffer);

            if (buffer.messageId == OpenRequest::MessageId) {
                auto request = IPC::extractMessage<OpenRequest>(buffer);
                handleOpenRequest(request, processes, openDescriptors);
                
            }
            else if (buffer.messageId == CreateRequest::MessageId) {
                auto request = IPC::extractMessage<CreateRequest>(buffer);
                handleCreateRequest(request, processes, openDescriptors);
            }
            else if (buffer.messageId == ReadRequest::MessageId) {
                auto request = IPC::extractMessage<ReadRequest>(buffer);
                //openDescriptors[0].read(request.senderTaskId);
                handleReadRequest(request, processes, openDescriptors);
            }
            else if (buffer.messageId == WriteRequest::MessageId) {
                auto request = IPC::extractMessage<WriteRequest>(buffer);
                handleWriteRequest(request, processes, openDescriptors);
                //ArgBuffer args{request.buffer, sizeof(request.buffer)};
                //openDescriptors[0].write(request.senderTaskId, args);
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