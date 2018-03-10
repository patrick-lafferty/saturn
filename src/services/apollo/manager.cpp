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

    void LayoutVisitor::visit(Tile& tile) {
        tile.bounds = bounds;
        updateBounds();
    }

    void LayoutVisitor::visit(Container* container) {
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

    void renderTile(uint32_t volatile* frameBuffer, Tile& tile, uint32_t displayWidth, Bounds dirty) {
        auto endY = dirty.y + std::min(tile.bounds.height, dirty.height);
        auto endX = dirty.x + std::min(tile.bounds.width, dirty.width);
        auto windowBuffer = tile.handle.buffer->buffer;

        for (auto y = dirty.y; y < endY; y++) {
            auto windowOffset = y * tile.bounds.width;
            auto screenOffset = tile.bounds.x + (tile.bounds.y + y) * displayWidth;

            void* dest = const_cast<uint32_t*>(frameBuffer) + screenOffset;
            void* source = windowBuffer + windowOffset;

            memcpy(dest, source,
                sizeof(uint32_t) * (endX - dirty.x)); 
        }
    }

    void Container::addChild(Tile tile) {
        tile.parent = this;
        children.push_back(tile);
        activeTaskId = tile.handle.taskId;
        layoutChildren();
    }

    void Container::addChild(Container* container) {
        children.push_back(container);
        container->parent = this;
        layoutChildren();
    }

    void Container::layoutChildren() {
        /*
        TODO: need to send a resize message to all tiles
        */
        Bounds childBounds;

        if (split == Split::Vertical) {
            childBounds.width = bounds.width / children.size();
            childBounds.height = bounds.height;
        }
        else {
            childBounds.width = bounds.width;
            childBounds.height = bounds.height / children.size();
        }

        LayoutVisitor visitor{childBounds, split};

        for (auto& child : children) {
            if (std::holds_alternative<Tile>(child)) {
                visitor.visit(std::get<Tile>(child));
            }
            else {
                auto container = std::get<Container*>(child);
                visitor.visit(container);
            }
        }
    }

    uint32_t Container::getChildrenCount() {
        return children.size();
    }

    std::optional<Tile*> Container::findTile(uint32_t taskId) {
        for (auto& child : children) {
            if (std::holds_alternative<Tile>(child)) {
                auto& tile = std::get<Tile>(child);

                if (tile.handle.taskId == taskId) {
                    return &tile;
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

    void Container::composite(uint32_t volatile* frameBuffer, uint32_t displayWidth) {
        for (auto& child : children) {
           if (std::holds_alternative<Tile>(child)) {
                auto& tile = std::get<Tile>(child); 
                renderTile(frameBuffer, tile, displayWidth, {0, 0, tile.bounds.width, tile.bounds.height});
           }
           else {
                auto container = std::get<Container*>(child); 
                container->composite(frameBuffer, displayWidth);
           }
        }
    }

    void Container::dispatchRenderMessages() {
       for (auto& child : children) {
           if (std::holds_alternative<Tile>(child)) {
                auto& tile = std::get<Tile>(child); 

                if (tile.canRender) {
                    tile.canRender = false;
                    Render render;
                    render.recipientId = tile.handle.taskId;
                    send(IPC::RecipientType::TaskId, &render);
                }
           }
           else {
                auto container = std::get<Container*>(child); 
                container->dispatchRenderMessages();
           }
        } 
    }

    Display::Display(Bounds screenBounds)
        : screenBounds {screenBounds} {
        root = new Container;
        root->split = Split::Horizontal;
        activeContainer = root;
        root->bounds = screenBounds;
    }

    void Display::addTile(Tile tile) {
        activeContainer->addChild(tile);
    }

    bool Display::enableRendering(uint32_t taskId) {

        if (auto tile = root->findTile(taskId)) {
            tile.value()->canRender = true;

            CreateWindowSucceeded success;
            success.recipientId = taskId;
            send(IPC::RecipientType::TaskId, &success);

            return true;
        }

        return false;
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
        auto maybeTile = root->findTile(taskId);

        if (maybeTile) { 
            auto& tile = *maybeTile.value();
            renderTile(frameBuffer, tile, screenBounds.width, dirty);
            tile.canRender = true;
        }
        
    }

    void Display::renderAll(uint32_t volatile* frameBuffer) {
        root->composite(frameBuffer, screenBounds.width);
    }

    void Display::splitContainer(Split split) {
        //activeContainer->
    }

    void Display::render() {
        root->dispatchRenderMessages();
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

        /*
        Display 0 is capcom
        */
        displays.push_back({{0, 0, 800, 600}});
        /*
        Display 1 is the main display
        */
        displays.push_back({{0, 0, 800, 600}});
    }

    void Manager::handleCreateWindow(const CreateWindow& message) {
        auto windowBuffer = new WindowBuffer;
        auto backgroundColour = 0x00'20'20'20;
        memset(windowBuffer->buffer, backgroundColour, screenWidth * screenHeight * 4);

        WindowHandle handle {windowBuffer, message.senderTaskId};
        Bounds bounds {0, 0, message.width, message.height};

        if (message.senderTaskId == capcomTaskId) {
            displays[0].addTile({bounds, handle});
        }
        else {
            displays[currentDisplay].addTile({bounds, handle});
        }

        ShareMemory share;
        share.ownerAddress = reinterpret_cast<uintptr_t>(windowBuffer);
        share.sharedAddress = message.bufferAddress;
        share.sharedTaskId = message.senderTaskId;
        share.size = 800 * 600 * 4;

        send(IPC::RecipientType::ServiceRegistryMailbox, &share);
    }

    void Manager::handleUpdate(const Update& message) {
        
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

    void Manager::handleSplitContainer(const SplitContainer& message) {
        auto index = currentDisplay;

        if (index == 0) {
            index = previousDisplay;
        }

        displays[index].splitContainer(message.direction);
    }

    void Manager::handleLaunchProgram(const LaunchProgram& message) {
        launch("/bin/dsky.bin");
    }

    void Manager::handleHideOverlay() {
        currentDisplay = previousDisplay;
        displays[currentDisplay].renderAll(linearFrameBuffer);
    }

    void Manager::handleShareMemoryResult(const ShareMemoryResult& message) {

        for(auto& display : displays) {
            if (display.enableRendering(message.sharedTaskId)) {
                break;
            }
        }
    }

    void Manager::handleKeyPress(Keyboard::KeyPress& message) {
        switch (message.key) {
            case Keyboard::VirtualKey::F1: {
                
                showCapcom = !showCapcom;

                if (showCapcom) {
                    displays[0].renderAll(linearFrameBuffer);
                    previousDisplay = currentDisplay;
                    currentDisplay = 0;
                }
                else {
                    handleHideOverlay();
                }

                break;
            }
            default: {
                displays[currentDisplay].injectKeypress(message);
                break;
            }
        }
    }

    void Manager::messageLoop() {

        double time = Saturn::Time::getHighResolutionTimeSeconds(); 
        double accumulator = 0.;
        double desiredFrameTime = 1.f / 30.f;

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
                            case MessageId::SplitContainer: {
                                auto message = IPC::extractMessage<SplitContainer>(buffer);
                                handleSplitContainer(message);

                                break;
                            }
                            case MessageId::LaunchProgram: {
                                auto message = IPC::extractMessage<LaunchProgram>(buffer);
                                handleLaunchProgram(message);

                                break;
                            }
                            case MessageId::HideOverlay: {
                                handleHideOverlay();
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

                displays[currentDisplay].render();
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