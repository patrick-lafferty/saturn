/*
Copyright (c) 2018, Patrick Lafferty
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
#include "manager.h"
#include "messages.h"
#include <services.h>
#include <system_calls.h>
#include <vector>
#include <list>
#include <services/startup/startup.h>

using namespace Kernel;

namespace Window {

    uint32_t registerService() {
        RegisterService registerRequest;
        registerRequest.type = ServiceType::WindowManager;

        IPC::MaximumMessageBuffer buffer;

        send(IPC::RecipientType::ServiceRegistryMailbox, &registerRequest);
        receive(&buffer);

        if (buffer.messageNamespace == IPC::MessageNamespace::ServiceRegistry) {

            if (buffer.messageId == static_cast<uint32_t>(Kernel::MessageId::RegisterServiceDenied)) {
                return 0;
            }
            else if (buffer.messageId == static_cast<uint32_t>(Kernel::MessageId::VGAServiceMeta)) {

                auto msg = IPC::extractMessage<VGAServiceMeta>(buffer);

                NotifyServiceReady ready;
                send(IPC::RecipientType::ServiceRegistryMailbox, &ready);

                return msg.vgaAddress;
            }
        }

        return 0;
    }

    void launchShell() {
        //run("/applications/dsky/dsky.bin");
        uint32_t descriptor {0};
        auto result = openSynchronous("/bin/dsky.bin");

        auto entryPointResult = readSynchronous(result.fileDescriptor, 4);
        uint32_t entryPoint {0};
        memcpy(&entryPoint, entryPointResult.buffer, 4);

        if (entryPoint > 0) {
            auto pid = run(entryPoint);
        }
    }

    struct Window {
        WindowBuffer* buffer;
        uint32_t taskId;
    };

    int main() {
        auto vgaAddress = registerService();

        if (vgaAddress == 0) {
            return 1;
        }

        auto linearFrameBuffer = reinterpret_cast<uint32_t volatile*>(vgaAddress);
        std::vector<Window> windows;
        std::list<Window> windowsWaitingToShare;

        launchShell();

        while (true) {
            IPC::MaximumMessageBuffer buffer;
            receive(&buffer);

            switch (buffer.messageNamespace) {
                case IPC::MessageNamespace::WindowManager: {

                    switch (static_cast<MessageId>(buffer.messageId)) {
                        case MessageId::CreateWindow: {
                            auto message = IPC::extractMessage<CreateWindow>(buffer);
                            auto windowBuffer = new WindowBuffer;
                            memset(windowBuffer->buffer, 0, 800 * 600 * 4);

                            Window window {windowBuffer, buffer.senderTaskId};
                            windowsWaitingToShare.push_back(window);

                            ShareMemory share;
                            share.ownerAddress = reinterpret_cast<uintptr_t>(windowBuffer);
                            share.sharedAddress = message.bufferAddress;
                            share.sharedTaskId = message.senderTaskId;
                            share.size = 800 * 600 * 4;

                            send(IPC::RecipientType::ServiceRegistryMailbox, &share);

                            break;
                        }
                        case MessageId::Update: {

                            auto message = IPC::extractMessage<Update>(buffer);
                            uint32_t* windowBuffer = nullptr;

                            for(auto& it : windows) {
                                if (it.taskId == message.senderTaskId) {
                                    windowBuffer = it.buffer->buffer;
                                    break;
                                }
                            }

                            if (windowBuffer == nullptr) {
                                continue;
                            }

                            for (int y = message.y; y < message.height; y++) {
                                auto offset = y * 800;

                                for (int x = message.x; x < message.width; x++) {
                                    linearFrameBuffer[x + offset] = windowBuffer[x + offset];
                                }
                            }

                            break;
                        }
                    }

                    break;
                }
                case IPC::MessageNamespace::ServiceRegistry: {
                    
                    switch (static_cast<Kernel::MessageId>(buffer.messageId)) {
                        case Kernel::MessageId::ShareMemoryResult: {

                            auto message = IPC::extractMessage<ShareMemoryResult>(buffer);

                            auto it = windowsWaitingToShare.begin();
                            CreateWindowSucceeded success;
                            success.recipientId = 0;

                            while (it != windowsWaitingToShare.end()) {
                                if (it->taskId == message.sharedTaskId) {
                                    success.recipientId = it->taskId;
                                    windows.push_back(*it);
                                    windowsWaitingToShare.erase(it);
                                    break;
                                }

                                it.operator++();
                            }

                            if (success.recipientId != 0) {
                                send(IPC::RecipientType::TaskId, &success);
                            }

                            break;
                        }
                    }

                    break;
                }
            }
        }
    }
}