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

    void LayoutVisitor::operator()(Tile tile) {
        tile.bounds = bounds;
        updateBounds();
    }

    void LayoutVisitor::operator()(Container* container) {
        container->bounds = bounds;
        container->layoutChildren();
        updateBounds();
    }

    void LayoutVisitor::updateBounds() {

        switch (split) {
            case Split::Horizontal: {
                bounds.y += bounds.height;
                break;
            }
            case Split::Vertical: {
                bounds.x += bounds.width;
                break;
            }
        }
    }

    void Container::addChild(Tile tile) {
        children.push_back(tile);
        activeTaskId = tile.handle.taskId;
        tile.parent = this;
        layoutChildren();
    }

    void Container::addChild(Container* container) {
        children.push_back(container);
        container->parent = this;
        layoutChildren();
    }

    void Container::layoutChildren() {
        Bounds childBounds;

        if (split == Split::Horizontal) {
            childBounds.width = bounds.width / children.size();
            childBounds.height = bounds.height;
        }
        else {
            childBounds.width = bounds.width;
            childBounds.height = bounds.height / children.size();
        }

        for (auto& child : children) {
            std::visit(LayoutVisitor{childBounds, split}, child);
        }
    }

    uint32_t Container::getChildrenCount() {
        return children.size();
    }

    std::optional<Tile> Container::findTile(uint32_t taskId) {
        for (auto& child : children) {
            if (std::holds_alternative<Tile>(child)) {
                auto& tile = std::get<Tile>(child);

                if (tile.handle.taskId == taskId) {
                    return tile;
                }
            }
            else {
                auto container = std::get<Container*>(child);

                if (auto tile = container->findTile(taskId)) {
                    return tile;
                }
            }
        }

        return {};
    }

    Display::Display(Bounds screenBounds)
        : screenBounds {screenBounds} {
        activeContainer = &root;
    }

    void Display::addTile(Tile tile) {
        activeContainer->addChild(tile);
    }

    void Display::enableRendering(uint32_t taskId) {

        if (auto tile = root.findTile(taskId)) {
            tile->canRender = true;
        }
    }

    void Display::injectKeypress(Keyboard::KeyPress& message) {

        if (activeContainer->activeTaskId > 0) {
            message.recipientId = activeContainer->activeTaskId;
            send(IPC::RecipientType::TaskId, &message);
        }
    }

    void Display::injectCharacterInput(Keyboard::CharacterInput& message) {

       if (activeContainer->activeTaskId > 0) {
            message.recipientId = activeContainer->activeTaskId;
            send(IPC::RecipientType::TaskId, &message);
        } 
    }

    void Display::composite(uint32_t volatile* frameBuffer, uint32_t taskId, Bounds dirty) {
        auto maybeTile = root.findTile(taskId);

        if (maybeTile && maybeTile->canRender) {
            auto tile = maybeTile.value();
            auto endY = std::min(tile.bounds.height, std::min(screenBounds.height, dirty.height + dirty.y));
            auto endX = std::min(tile.bounds.width, std::min(screenBounds.width, dirty.width + dirty.x));
            auto windowBuffer = tile.handle.buffer->buffer;

            for (auto y = dirty.y; y < endY; y++) {
                auto windowOffset = y * tile.bounds.width;
                auto screenOffset = tile.bounds.x + (tile.bounds.y + y) * screenBounds.width;

                for (auto x = dirty.x; x < endX; x++) {
                    frameBuffer[x + screenOffset] = windowBuffer[x + windowOffset];
                }
            }
        }
    }

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

    uint32_t launch(const char* path) {
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

        displays.push_back({{0, 0, 800, 600}});
    }

    void Manager::handleCreateWindow(const CreateWindow& message) {
        auto windowBuffer = new WindowBuffer;
        auto backgroundColour = 0x00'20'20'20;
        memset(windowBuffer->buffer, backgroundColour, screenWidth * screenHeight * 4);

        WindowHandle handle {windowBuffer, message.senderTaskId};
        /*window.width = message.width;
        window.height = message.height;
        windowsWaitingToShare.push_back(window);*/
        Bounds bounds {0, 0, message.width, message.height};
        //tilesWaitingToShare.push_back({bounds, handle});
        displays[currentDisplay].addTile({bounds, handle});

        ShareMemory share;
        share.ownerAddress = reinterpret_cast<uintptr_t>(windowBuffer);
        share.sharedAddress = message.bufferAddress;
        share.sharedTaskId = message.senderTaskId;
        share.size = 800 * 600 * 4;

        send(IPC::RecipientType::ServiceRegistryMailbox, &share);
    }

    void Manager::handleUpdate(const Update& message) {
        /*for(auto& it : windows) {
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
        }*/
        Bounds dirty {message.x, message.y, message.width, message.height};
        displays[currentDisplay].composite(linearFrameBuffer, message.senderTaskId, dirty);
    }

    void Manager::handleMove(const Move& /*message*/) {
        /*for(auto& it : windows) {
            if (it.taskId == message.senderTaskId) {
                it.x = message.x;
                it.y = message.y;
                break;
            }
        }*/
    }

    void Manager::handleReadyToRender(const ReadyToRender& /*message*/) {
        /*for(auto& it : windows) {
            if (it.taskId == message.senderTaskId) {
                it.readyToRender = true;
                break;
            }
        }*/
    }

    void Manager::handleShareMemoryResult(const ShareMemoryResult& message) {

        CreateWindowSucceeded success;
        success.recipientId = 0;

        //TODO: check all displays
        displays[currentDisplay].enableRendering(message.senderTaskId);

        /*for (auto it = begin(tilesWaitingToShare); it != end(tilesWaitingToShare); ++it) {
            if (it->handle.taskId == message.sharedTaskId) {
                success.recipientId = it->taskId;
                tilesWaitingToShare.erase(it);

                auto& display = displays[currentDisplay];
                display.addChild(*it);

                if (it->handle.taskId == capcomTaskId) {
                    capcomWindowId = display.getChildrenCount() - 1;
                }
                else if (!hasFocus) {
                    activeWindow = display.getChildrenCount() - 1;
                    hasFocus = true;
                }

                break;
            }
        }*/


        if (success.recipientId != 0) {
            send(IPC::RecipientType::TaskId, &success);
        }
    }

    void Manager::handleKeyPress(Keyboard::KeyPress& message) {
        switch (message.key) {
            case Keyboard::VirtualKey::F1: {
                /*Update update;
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
                handleUpdate(update);*/
                capcom.visible = !capcom.visible;

                break;
            }
            default: {
                /*auto& activeDisplay = displays[currentDisplay];
                message.recipientId = windows[activeWindow].taskId;
                send(IPC::RecipientType::TaskId, &message);*/
                displays[currentDisplay].injectKeypress(message);
                break;
            }
        }
    }

    void Manager::messageLoop() {

        double time = Saturn::Time::getHighResolutionTimeSeconds(); 
        double accumulator = 0.;
        double desiredFrameTime = 1.f / 60.f;

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
                            default: {
                                printf("[WindowManager] Unhandled Window message id\n");
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
                            default: {
                                printf("[WindowManager] Unhandled service registry message id\n");
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

                                /*buffer.recipientId = windows[activeWindow].taskId;
                                send(IPC::RecipientType::TaskId, &buffer);*/
                                auto message = IPC::extractMessage<Keyboard::CharacterInput>(buffer);
                                displays[currentDisplay].injectCharacterInput(message);

                                break;
                            }
                            default: {
                                printf("[WindowManager] Unhandled keyboard message id\n");
                            }
                        }

                        break;
                    }
                    default: {
                    }
                }
            }

            while (accumulator >= desiredFrameTime) {
                accumulator -= desiredFrameTime;

                /*if (!windows.empty() && activeWindow < windows.size()) {
                    auto& window = windows[activeWindow];

                    if (!window.readyToRender) {
                        continue;
                    }

                    window.readyToRender = false;

                    Render render;
                    render.recipientId = windows[activeWindow].taskId;
                    send(IPC::RecipientType::TaskId, &render);
                }*/
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