#include "processFileSystem.h"
#include <services.h>
#include <system_calls.h>
#include <string.h>
#include <services/virtualFileSystem/virtualFileSystem.h>
#include <stdio.h>
#include "object.h"
#include <stdlib.h>
#include <vector>
#include <parsing>

using namespace Kernel;
using namespace VFS;
using namespace Vostok;

namespace PFS {

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

        auto words = split({request.path, strlen(request.path)}, '/');
        if (!words.empty() && words[0].compare("process") == 0) {

            if (words.size() > 1) {
                char pidString[11];
                memset(pidString, '\0', sizeof(pidString));
                words[1].copy(pidString, words[1].length());
                uint32_t pid = strtol(pidString, nullptr, 10);
                ProcessObject* process;
                auto found = findObject(processes, pid, &process);

                if (found) {

                    if (words.size() > 2) {
                        //path is /process/<pid>/<function or property>
                        auto functionId = process->getFunction(words[2]);

                        if (functionId >= 0) {
                            failed = false;
                            result.success = true;
                            openDescriptors.push_back({process, {static_cast<uint32_t>(functionId)}, DescriptorType::Function});
                            result.fileDescriptor = openDescriptors.size() - 1;
                        }
                        else {
                            auto propertyId = process->getProperty(words[2]);

                            if (propertyId >= 0) {
                                failed = false;
                                result.success = true;
                                openDescriptors.push_back({process, {static_cast<uint32_t>(propertyId)}, DescriptorType::Property});
                                result.fileDescriptor = openDescriptors.size() - 1;
                            }
                            
                        }
                    }
                    else {
                        //its the process object itself
                        failed = false;
                        result.success = true;
                        openDescriptors.push_back({process, {}, DescriptorType::Object});
                        result.fileDescriptor = openDescriptors.size() - 1;
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

        auto words = split({request.path, strlen(request.path)}, '/');

        if (words.size() == 2 && words[0].compare("process") == 0) {

            char pidString[11];
            memset(pidString, '\0', sizeof(pidString));
            words[1].copy(pidString, words[1].length());
            uint32_t pid = strtol(pidString, nullptr, 10);
            ProcessObject* process;
            auto found = findObject(processes, pid, &process);

            if (!found) {
                ProcessObject p {pid};
                processes.push_back(p);
                failed = false;
                result.success = true;
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

            if (descriptor.instance == nullptr) {
                //its the main /process thing, return a list of all objects

                for (auto& desc : openDescriptors) {
                    if (desc.instance != nullptr)  {
                        args.writeValueWithType(static_cast<ProcessObject*>(desc.instance)->pid, ArgTypes::Uint32);
                    }
                }

            }
            else {
                //its a process object, return a summary of it

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

        MountRequest request;
        const char* path = "/process";
        memcpy(request.path, path, strlen(path) + 1);
        request.serviceType = ServiceType::VFS;

        send(IPC::RecipientType::ServiceName, &request);

        messageLoop();
    }
}