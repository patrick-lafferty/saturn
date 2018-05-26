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

    int EventSystem::getFunction(std::string_view name) {
        if (name.compare("subscribe") == 0) {
            return static_cast<int>(FunctionId::Subscribe);
        }

        return -1;
    }

    void EventSystem::readFunction(uint32_t requesterTaskId, uint32_t requestId, uint32_t functionId) {
        describeFunction(requesterTaskId, requestId, functionId);
    }

    void EventSystem::writeFunction(uint32_t requesterTaskId, uint32_t requestId, uint32_t functionId, Vostok::ArgBuffer& args) {
        auto type = args.readType();

        if (type != Vostok::ArgTypes::Function) {
            Vostok::replyWriteSucceeded(requesterTaskId, requestId, false);
            return;
        }

        switch(functionId) {
            case static_cast<uint32_t>(FunctionId::Subscribe): {
                auto subscriberTaskId = args.read<uint32_t>(Vostok::ArgTypes::Uint32);

                if (!args.hasErrors()) {
                    subscribe(requesterTaskId, requestId, subscriberTaskId);                
                }
                else {
                    Vostok::replyWriteSucceeded(requesterTaskId, requestId, false);
                }

                break;
            }
            default: {
                Vostok::replyWriteSucceeded(requesterTaskId, requestId, false);
            }
        }
    }

    void EventSystem::describeFunction(uint32_t requesterTaskId, uint32_t requestId, uint32_t functionId) {
        ReadResult result {};
        result.requestId = requestId;
        result.success = true;
        Vostok::ArgBuffer args{result.buffer, sizeof(result.buffer)};
        args.writeType(Vostok::ArgTypes::Function);
        
        switch(functionId) {
            case static_cast<uint32_t>(FunctionId::Subscribe): {
                args.writeType(Vostok::ArgTypes::Uint32);
                args.writeType(Vostok::ArgTypes::EndArg);
                break;
            }
            default: {
                result.success = false;
            }
        }

        result.recipientId = requesterTaskId;
        send(IPC::RecipientType::TaskId, &result);
    }

    void EventSystem::subscribe(uint32_t requesterTaskId, uint32_t requestId, uint32_t subscriberTaskId) {
        char path[256];
        memset(path, '\0', 256);
        sprintf(path, "/applications/%d/receive", subscriberTaskId);

        auto result = openSynchronous(path);

        if (result.success) {
            subscribers.push_back(result.fileDescriptor);

            if (!receiveSignature) {
                auto readResult = readSynchronous(result.fileDescriptor, 0);
                receiveSignature = ReceiveSignature {{}, {nullptr, 0}};
                auto& sig = receiveSignature.value();
                sig.result = readResult;
                sig.args = Vostok::ArgBuffer{&sig.result.buffer[0], 256};
            }
        }

        Vostok::replyWriteSucceeded(requesterTaskId, requestId, result.success);
    }

    void EventSystem::handleOpenRequest(OpenRequest& request) {
        OpenResult result;
        result.requestId = request.requestId;
        result.serviceType = Kernel::ServiceType::VFS;

        auto words = split({request.path, strlen(request.path)}, '/');

        if (words.size() == 1) {
            
            if (words[0].compare("subscribe") == 0) {
                Vostok::FileDescriptor descriptor;
                descriptor.instance = this;
                descriptor.functionId = 0;
                descriptor.type = Vostok::DescriptorType::Function;

                auto id = nextDescriptorId++;
                openDescriptors.push_back(FileDescriptor{id, descriptor});
                result.fileDescriptor = id;
                result.success = true;
            }
            else {

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

    void EventSystem::handleReadRequest(ReadRequest& request) {

        auto it = std::find_if(begin(openDescriptors), end(openDescriptors),
            [&](const auto& descriptor) {
                return descriptor.id == request.fileDescriptor;
            });

        if (it != end(openDescriptors)) {
            if (std::holds_alternative<Vostok::FileDescriptor>(it->object)) {
                auto& desc = std::get<Vostok::FileDescriptor>(it->object);
                desc.read(request.senderTaskId, request.requestId);
            }
        }
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

            if (std::holds_alternative<Log*>(it->object)) {
                auto log = std::get<Log*>(it->object);
                std::string_view view {reinterpret_cast<char*>(&request.buffer[0]), request.writeLength};
                log->addEntry(view);

                if (!subscribers.empty()) {
                    broadcastEvent(view);
                }
            }
            else if (std::holds_alternative<Vostok::FileDescriptor>(it->object)) {
                auto descriptor = std::get<Vostok::FileDescriptor>(it->object);
                Vostok::ArgBuffer args{request.buffer, sizeof(request.buffer)};
                descriptor.write(request.senderTaskId, request.requestId, args);
                return;
            }

        }

        send(IPC::RecipientType::TaskId, &result);
    }

    void EventSystem::broadcastEvent(std::string_view event) {
        auto signature = receiveSignature.value();
        signature.args.typedBuffer = signature.result.buffer;
        signature.args.readType();
        signature.args.write(event.data(), Vostok::ArgTypes::Cstring);

        for (auto subscriber : subscribers) {
            write(subscriber, signature.result.buffer, sizeof(signature.result.buffer));
        }
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
                            auto request = IPC::extractMessage<ReadRequest>(buffer);
                            handleReadRequest(request);
                            break;
                        }
                        case MessageId::WriteRequest: {
                            auto request = IPC::extractMessage<WriteRequest>(buffer);
                            handleWriteRequest(request);
                            break;
                        }
                        case MessageId::WriteResult: {
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