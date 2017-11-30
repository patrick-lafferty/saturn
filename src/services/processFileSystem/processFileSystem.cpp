#include "processFileSystem.h"
#include <services.h>
#include <system_calls.h>
#include <string.h>
#include <services/virtualFileSystem/virtualFileSystem.h>
#include <stdio.h>
#include "object.h"
#include <stdlib.h>
#include <vector>

using namespace Kernel;
using namespace VFS;
using namespace Vostok;

namespace PFS {

    /*
    You can either open
    1) the VostokObject itself, reading it does something like returning a default
        property important to the object
    2) a function, reading returns the function's signature and writing
        calls the function
    3) a property, reading returns the value and writing sets the value
    */
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

    bool findObject(std::vector<ProcessObject>& objects, uint32_t pid, ProcessObject** found) {
        for (auto& object : objects) {
            if (object.pid == pid) {
                *found = &object;
                return true;
            }
        }

        return false;
    }

    void handleOpenRequest(OpenRequest& request, std::vector<ProcessObject>& processes, std::vector<FileDescriptor>& openDescriptors) {
        bool failed {true};
        OpenResult result{};
        result.serviceType = ServiceType::VFS;

        //TODO: replace this string parsing with a proper C++ split()
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
                            result.success = true;
                            openDescriptors.push_back({process, {static_cast<uint32_t>(functionId)}, DescriptorType::Function});
                            result.fileDescriptor = openDescriptors.size() - 1;
                        }
                        else {
                            auto propertyId = process->getProperty(start);

                            if (propertyId >= 0) {
                                failed = false;
                                result.success = true;
                                openDescriptors.push_back({process, {static_cast<uint32_t>(propertyId)}, DescriptorType::Property});
                                result.fileDescriptor = openDescriptors.size() - 1;
                            }
                            
                        }
                    }
                }
            }
            else {
                failed = false;
                result.success = true;
                openDescriptors.push_back({nullptr, {}, DescriptorType::Object});
                result.fileDescriptor = openDescriptors.size() - 1;
            }
        }

        if (failed) {
            result.success = false;
        }

        send(IPC::RecipientType::ServiceName, &result);
    }

    void handleCreateRequest(CreateRequest& request, std::vector<ProcessObject>& processes) {
        bool failed {true};
        CreateResult result{};
        result.serviceType = ServiceType::VFS;

        //TODO: replace this string parsing with a proper C++ split()
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
                    processes.push_back(p);
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

    void handleReadRequest(ReadRequest& request, std::vector<FileDescriptor>& openDescriptors) {
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

    void handleWriteRequest(WriteRequest& request, std::vector<FileDescriptor>& openDescriptors) {
        ArgBuffer args{request.buffer, sizeof(request.buffer)};
        openDescriptors[request.fileDescriptor].write(request.senderTaskId, args);
    }

    void messageLoop() {

        std::vector<ProcessObject> processes;
        std::vector<FileDescriptor> openDescriptors;

        while (true) {
            IPC::MaximumMessageBuffer buffer;
            receive(&buffer);

            if (buffer.messageId == OpenRequest::MessageId) {
                auto request = IPC::extractMessage<OpenRequest>(buffer);
                handleOpenRequest(request, processes, openDescriptors);
            }
            else if (buffer.messageId == CreateRequest::MessageId) {
                auto request = IPC::extractMessage<CreateRequest>(buffer);
                handleCreateRequest(request, processes);
            }
            else if (buffer.messageId == ReadRequest::MessageId) {
                auto request = IPC::extractMessage<ReadRequest>(buffer);
                handleReadRequest(request, openDescriptors);
            }
            else if (buffer.messageId == WriteRequest::MessageId) {
                auto request = IPC::extractMessage<WriteRequest>(buffer);
                handleWriteRequest(request, openDescriptors);
            }
        }
    }

    void service() {

        waitForServiceRegistered(ServiceType::VFS);

        MountRequest request{};
        const char* path = "/process";
        memcpy(request.path, path, strlen(path) + 1);
        request.serviceType = ServiceType::VFS;

        send(IPC::RecipientType::ServiceName, &request);

        messageLoop();
    }
}