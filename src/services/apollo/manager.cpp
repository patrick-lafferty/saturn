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
#include "lib/window.h"
#include <services.h>
#include <system_calls.h>
#include <services/startup/startup.h>
#include <algorithm>
#include <services/keyboard/messages.h>
#include <saturn/time.h>

using namespace Kernel;

namespace Apollo {

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

                Keyboard::RedirectToWindowManager redirect;
                redirect.serviceType = ServiceType::Keyboard;
                send(IPC::RecipientType::ServiceName, &redirect);

                return msg.vgaAddress;
            }
        }

        return 0;
    }

    uint32_t launch(char* path) {
        uint32_t descriptor {0};
        auto result = openSynchronous(path);

        auto entryPointResult = readSynchronous(result.fileDescriptor, 4);
        uint32_t entryPoint {0};
        memcpy(&entryPoint, entryPointResult.buffer, 4);

        if (entryPoint > 0) {
            auto pid = run(entryPoint);
            return pid;
        }

        return 0;
    }

    Manager::Manager(uint32_t framebufferAddress) {
        linearFrameBuffer = reinterpret_cast<uint32_t volatile*>(framebufferAddress);

        launch("/bin/dsky.bin");
        capcomTaskId = launch("/bin/capcom.bin");
    }

    void Manager::handleCreateWindow(const CreateWindow& message) {
        auto windowBuffer = new WindowBuffer;
        auto backgroundColour = 0x00'20'20'20;
        memset(windowBuffer->buffer, backgroundColour, screenWidth * screenHeight * 4);

        WindowHandle window {windowBuffer, message.senderTaskId};
        window.width = message.width;
        window.height = message.height;
        windowsWaitingToShare.push_back(window);

        ShareMemory share;
        share.ownerAddress = reinterpret_cast<uintptr_t>(windowBuffer);
        share.sharedAddress = message.bufferAddress;
        share.sharedTaskId = message.senderTaskId;
        share.size = 800 * 600 * 4;

        send(IPC::RecipientType::ServiceRegistryMailbox, &share);
    }

    void Manager::handleUpdate(const Update& message) {
        for(auto& it : windows) {
            if (it.taskId == message.senderTaskId) {
                it.readyToRender = true;

                auto endY = std::min(it.height, std::min(screenHeight, message.height + message.y));
                auto endX = std::min(it.width, std::min(screenWidth, message.width + message.x));
                auto windowBuffer = it.buffer->buffer;

                for (auto y = message.y; y < endY; y++) {
                    auto windowOffset = y * it.width;
                    auto screenOffset = it.x + (it.y + y) * screenWidth;

                    for (auto x = message.x; x < endX; x++) {
                        linearFrameBuffer[x + screenOffset] = windowBuffer[x + windowOffset];
                    }
                }

                break;
            }
        }
    }

    void Manager::handleMove(const Move& message) {
        for(auto& it : windows) {
            if (it.taskId == message.senderTaskId) {
                it.x = message.x;
                it.y = message.y;
                break;
            }
        }
    }

    void Manager::handleReadyToRender(const ReadyToRender& message) {
        for(auto& it : windows) {
            if (it.taskId == message.senderTaskId) {
                it.readyToRender = true;
                break;
            }
        }
    }

    void Manager::handleShareMemoryResult(const ShareMemoryResult& message) {
        auto it = windowsWaitingToShare.begin();
        CreateWindowSucceeded success;
        success.recipientId = 0;

        while (it != windowsWaitingToShare.end()) {
            if (it->taskId == message.sharedTaskId) {
                success.recipientId = it->taskId;
                windows.push_back(*it);
                windowsWaitingToShare.erase(it);

                if (it->taskId == capcomTaskId) {
                    capcomWindowId = windows.size() - 1;
                }

                break;
            }

            it.operator++();
        }

        if (success.recipientId != 0) {
            send(IPC::RecipientType::TaskId, &success);
        }
    }

    void Manager::handleKeyPress(Keyboard::KeyPress& message) {
        switch (message.key) {
            case Keyboard::VirtualKey::F1: {
                Update update;
                auto& oldWindow = windows[activeWindow];
                update.senderTaskId = oldWindow.taskId;
                update.x = oldWindow.x;
                update.y = oldWindow.y;
                update.width = oldWindow.width;
                update.height = oldWindow.height;

                if (activeWindow != capcomWindowId) {
                    previousActiveWindow = activeWindow;
                    activeWindow = capcomWindowId;
                    Show show;
                    show.recipientId = capcomTaskId;
                    send(IPC::RecipientType::TaskId, &show);
                }
                else {
                    activeWindow = previousActiveWindow;
                    Show show;
                    show.recipientId = windows[activeWindow].taskId;
                    send(IPC::RecipientType::TaskId, &show);
                }

                update.senderTaskId = windows[activeWindow].taskId;
                handleUpdate(update);

                break;
            }
            default: {
                message.recipientId = windows[activeWindow].taskId;
                send(IPC::RecipientType::TaskId, &message);
                break;
            }
        }
    }

    void Manager::messageLoop() {

        double time = Saturn::Time::getHighResolutionTimeSeconds(); 
        double accumulator = 0.;
        double desiredFrameTime = 0.5;//1.f / 60.f;

        while (true) {

            auto currentTime = Saturn::Time::getHighResolutionTimeSeconds();
            accumulator += (currentTime - time);
            time = currentTime;

            IPC::MaximumMessageBuffer buffer;

            while (peekReceive(&buffer)) {
                switch (buffer.messageNamespace) {
                    case IPC::MessageNamespace::WindowManager: {

                        switch (static_cast<MessageId>(buffer.messageId)) {
                            case MessageId::CreateWindow: {
                                auto message = IPC::extractMessage<CreateWindow>(buffer);
                                handleCreateWindow(message);

                                break;
                            }
                            case MessageId::ReadyToRender: {
                                auto message = IPC::extractMessage<ReadyToRender>(buffer);
                                handleReadyToRender(message);

                                break;
                            }
                            case MessageId::Update: {

                                auto message = IPC::extractMessage<Update>(buffer);
                                handleUpdate(message); 

                                break;
                            }
                            case MessageId::Move: {
                                auto message = IPC::extractMessage<Move>(buffer);
                                handleMove(message);

                                break;
                            }
                        }

                        break;
                    }
                    case IPC::MessageNamespace::ServiceRegistry: {
                        
                        switch (static_cast<Kernel::MessageId>(buffer.messageId)) {
                            case Kernel::MessageId::ShareMemoryResult: {

                                auto message = IPC::extractMessage<ShareMemoryResult>(buffer);
                                handleShareMemoryResult(message); 

                                break;
                            }
                        }

                        break;
                    }
                    case IPC::MessageNamespace::Keyboard: {
                        switch (static_cast<Keyboard::MessageId>(buffer.messageId)) {
                            case Keyboard::MessageId::KeyPress: {

                                auto message = IPC::extractMessage<Keyboard::KeyPress>(buffer);
                                handleKeyPress(message);                            
                                
                                break;
                            }
                            case Keyboard::MessageId::CharacterInput: {

                                buffer.recipientId = windows[activeWindow].taskId;
                                send(IPC::RecipientType::TaskId, &buffer);

                                break;
                            }
                        }

                        break;
                    }
                }
            }

            while (accumulator >= desiredFrameTime) {
                accumulator -= desiredFrameTime;

                if (!windows.empty() && activeWindow < windows.size()) {
                    auto& window = windows[activeWindow];

                    if (!window.readyToRender) {
                        continue;
                    }

                    window.readyToRender = false;

                    Render render;
                    render.recipientId = windows[activeWindow].taskId;
                    send(IPC::RecipientType::TaskId, &render);
                }
            }
        }
    }

    int main() {
        auto framebufferAddress = registerService();

        if (framebufferAddress == 0) {
            return 1;
        }

        Manager manager {framebufferAddress};
        manager.messageLoop();

        return 0;
    }
}