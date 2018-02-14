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
#include "processFileSystem.h"
#include <services.h>
#include <system_calls.h>
#include <string.h>
#include <services/virtualFileSystem/virtualFileSystem.h>
#include <stdio.h>
#include "object.h"
#include <stdlib.h>
#include <vector>
#include <saturn/parsing.h>

using namespace VirtualFileSystem;
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
        OpenResult result;
        result.requestId = request.requestId;
        result.serviceType = Kernel::ServiceType::VFS;

        auto addDescriptor = [&](auto instance, auto id, auto type) {
            failed = false;
            result.success = true;
            openDescriptors.push_back({instance, {static_cast<uint32_t>(id)}, type});
            result.fileDescriptor = openDescriptors.size() - 1;
        };

        auto words = split({request.path, strlen(request.path)}, '/');

        if (words.size() > 0) {
            char pidString[11];
            memset(pidString, '\0', sizeof(pidString));
            words[0].copy(pidString, words[1].length());
            uint32_t pid = strtol(pidString, nullptr, 10);
            ProcessObject* process;
            auto found = findObject(processes, pid, &process);

            if (found) {

                if (words.size() > 1) {
                    //path is /process/<pid>/<function or property>
                    auto functionId = process->getFunction(words[1]);

                    if (functionId >= 0) {
                        addDescriptor(process, functionId, DescriptorType::Function);
                    }
                    else {
                        auto propertyId = process->getProperty(words[1]);

                        if (propertyId >= 0) {
                            addDescriptor(process, propertyId, DescriptorType::Property);
                        }
                    }
                }
                else {
                    //its the process object itself
                    addDescriptor(process, 0, DescriptorType::Object);
                }
            }
        }
        else {
            addDescriptor(nullptr, 0, DescriptorType::Object);
        }

        if (failed) {
            result.success = false;
        }

        send(IPC::RecipientType::ServiceName, &result);
    }

    void handleCreateRequest(CreateRequest& request, std::vector<ProcessObject>& processes) {
        bool failed {true};
        CreateResult result;
        result.requestId = request.requestId;
        result.serviceType = Kernel::ServiceType::VFS;

        auto words = split({request.path, strlen(request.path)}, '/');

        char pidString[11];
        memset(pidString, '\0', sizeof(pidString));
        words[0].copy(pidString, words[0].length());
        uint32_t pid = strtol(pidString, nullptr, 10);
        ProcessObject* process;
        auto found = findObject(processes, pid, &process);

        if (!found) {
            ProcessObject p {pid};
            processes.push_back(p);
            failed = false;
            result.success = true;
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
            result.requestId = request.requestId;
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
            descriptor.read(request.senderTaskId, request.requestId);
        }
    }

    void handleWriteRequest(WriteRequest& request, std::vector<FileDescriptor>& openDescriptors) {
        ArgBuffer args{request.buffer, sizeof(request.buffer)};
        openDescriptors[request.fileDescriptor].write(request.senderTaskId, request.requestId, args);
    }

    void messageLoop() {

        std::vector<ProcessObject> processes;
        std::vector<FileDescriptor> openDescriptors;

        while (true) {
            IPC::MaximumMessageBuffer buffer;
            receive(&buffer);

            switch(buffer.messageNamespace) {
                case IPC::MessageNamespace::VFS: {
                    switch(static_cast<MessageId>(buffer.messageId)) {
                        case MessageId::OpenRequest: {
                            auto request = IPC::extractMessage<OpenRequest>(buffer);
                            handleOpenRequest(request, processes, openDescriptors);
                            break;
                        }
                        case MessageId::CreateRequest: {
                            auto request = IPC::extractMessage<CreateRequest>(buffer);
                            handleCreateRequest(request, processes);
                            break;
                        }
                        case MessageId::ReadRequest: {
                            auto request = IPC::extractMessage<ReadRequest>(buffer);
                            handleReadRequest(request, openDescriptors);
                            break;
                        }
                        case MessageId::WriteRequest: {
                            auto request = IPC::extractMessage<WriteRequest>(buffer);
                            handleWriteRequest(request, openDescriptors);
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

    void service() {

        waitForServiceRegistered(Kernel::ServiceType::VFS);

        MountRequest request;
        const char* path = "/process";
        memcpy(request.path, path, strlen(path) + 1);
        request.serviceType = Kernel::ServiceType::VFS;
        request.cacheable = false;

        send(IPC::RecipientType::ServiceName, &request);

        messageLoop();
    }
}