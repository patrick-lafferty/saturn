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

#include "system.h"
#include "ipc.h"
#include <services.h>
#include <system_calls.h>
#include <saturn/wait.h>
#include <saturn/parsing.h>

using namespace VirtualFileSystem;

namespace Event {

    void Log::addEntry(std::string_view entry) {
        entries.push_back(std::string{entry});
    }

    void EventSystem::handleOpenRequest(OpenRequest& request) {
        OpenResult result;
        result.requestId = request.requestId;
        result.serviceType = Kernel::ServiceType::VFS;

        auto words = split({request.path, strlen(request.path)}, '/');

        if (words.size() == 1) {
            auto it = std::find_if(begin(logs), end(logs), [&](const auto& log) {
                return words[0].compare(log->name) == 0;
            });

            if (it != end(logs)) {
                FileDescriptor descriptor {nextDescriptorId++, it->get()};
                openDescriptors.push_back(descriptor);
                result.fileDescriptor = descriptor.id;
                result.success = true;
            }
        }

        send(IPC::RecipientType::ServiceName, &result);
    }

    void EventSystem::handleCreateRequest(CreateRequest& request) {

        auto words = split({request.path, strlen(request.path)}, '/');
        CreateResult result;
        result.requestId = request.requestId;
        result.serviceType = Kernel::ServiceType::VFS;

        if (words.size() == 1) {
            auto it = std::find_if(begin(logs), end(logs), [&](const auto& log) {
                return words[0].compare(log->name) == 0;
            });

            if (it == end(logs)) {
                logs.push_back(std::make_unique<Log>(words[0]));
                result.success = true;
            }
        }

        send(IPC::RecipientType::ServiceName, &result);
    }

    void EventSystem::handleWriteRequest(WriteRequest& request) {

        WriteResult result;
        result.requestId = request.requestId;
        result.serviceType = Kernel::ServiceType::VFS;
        result.recipientId = request.senderTaskId;

        auto it = std::find_if(begin(openDescriptors), end(openDescriptors), [&](auto& d) {
            return d.id == request.fileDescriptor;
        });

        if (it != end(openDescriptors)) {
            result.success = true;
            std::string_view view {reinterpret_cast<char*>(&request.buffer[0], request.writeLength)};
            it->log->addEntry(view);
        }

        send(IPC::RecipientType::TaskId, &result);
    }

    void EventSystem::messageLoop() {
        while (true) {
            IPC::MaximumMessageBuffer buffer;
            receive(&buffer);

            switch (buffer.messageNamespace) {
                case IPC::MessageNamespace::VFS: {
                    switch (static_cast<MessageId>(buffer.messageId)) {
                        case MessageId::OpenRequest: {
                            auto request = IPC::extractMessage<OpenRequest>(buffer);
                            handleOpenRequest(request);
                            break;
                        }
                        case MessageId::CreateRequest: {
                            auto request = IPC::extractMessage<CreateRequest>(buffer);
                            handleCreateRequest(request);
                            break;
                        }
                        case MessageId::ReadRequest: {
                            break;
                        }
                        case MessageId::WriteRequest: {
                            auto request = IPC::extractMessage<WriteRequest>(buffer);
                            handleWriteRequest(request);
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
        const char* path = "/events";
        memcpy(request.path, path, strlen(path) + 1);
        request.serviceType = Kernel::ServiceType::VFS;
        request.cacheable = false;

        send(IPC::RecipientType::ServiceName, &request);

        auto system = new EventSystem();
        system->messageLoop();
    }
}