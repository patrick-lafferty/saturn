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
#include <saturn/parsing.h>
#include <services/virtualFileSystem/vostok.h>

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
                bounds.y += 5;
                break;
            }
            case Split::Vertical: {
                bounds.x += bounds.width;
                bounds.x += 5;
                break;
            }
        }
    }

    void renderTile(uint32_t volatile* frameBuffer, Tile& tile, uint32_t displayWidth, Bounds dirty) {
        auto endY = dirty.y + std::min(tile.bounds.height, dirty.height);
        auto endX = dirty.x + std::min(tile.bounds.width, dirty.width);
        auto windowBuffer = tile.handle.buffer->buffer;

        for (auto y = dirty.y; y < endY; y++) {
            auto windowOffset = y * tile.stride;//tile.bounds.width;
            auto screenOffset = tile.bounds.x + (tile.bounds.y + y) * displayWidth;

            void* dest = const_cast<uint32_t*>(frameBuffer) + screenOffset;
            void* source = windowBuffer + windowOffset;

            memcpy(dest, source,
                sizeof(uint32_t) * (endX - dirty.x)); 
        }
    }

    void Container::addChild(Tile tile, Size size, bool focusable) {
        tile.parent = this;
        children.push_back({size, focusable, tile});
        activeTaskId = tile.handle.taskId;
        layoutChildren();
    }

    void Container::addChild(Container* container, Size size) {
        children.push_back({size, false, container});
        container->parent = this;
        layoutChildren();
    }

    void placeChild(Split split, ContainerChild& child, Bounds& childBounds, int gapSize) {
        
        if (split == Split::Vertical) {
            childBounds.width = child.size.actualSpace;
        }
        else {
            childBounds.height = child.size.actualSpace;
        }

        if (std::holds_alternative<Tile>(child.child)) {
            auto& tile = std::get<Tile>(child.child);
            tile.bounds = childBounds;
            Resize resize;
            resize.width = childBounds.width;
            resize.height = childBounds.height;
            resize.recipientId = tile.handle.taskId;
            send(IPC::RecipientType::TaskId, &resize);
        }
        else {
            auto container = std::get<Container*>(child.child);
            container->bounds = childBounds;
            container->layoutChildren();
        }

        switch (split) {
            case Split::Horizontal: {
                childBounds.y += childBounds.height;
                childBounds.y += gapSize;
                break;
            }
            case Split::Vertical: {
                childBounds.x += childBounds.width;
                childBounds.x += gapSize;
                break;
            }
        }
    }

    void Container::layoutChildren() {
        if (children.empty()) {
            return;
        }

        int unallocatedSpace {0};
        Bounds childBounds {bounds.x, bounds.y};

        if (split == Split::Vertical) {
            unallocatedSpace = bounds.width;
            childBounds.height = bounds.height;
        }
        else {
            unallocatedSpace = bounds.height;
            childBounds.width = bounds.width;
        }

        const auto gapSize = 5;
        unallocatedSpace -= gapSize * (children.size() - 1);

        int totalProportionalUnits = 0;

        for (auto& child : children) {
            if (child.size.unit == Unit::Fixed) {
                if (unallocatedSpace > 0) {
                    auto space = std::min(unallocatedSpace, child.size.desiredSpace);
                    child.size.actualSpace = space;
                    unallocatedSpace -= space;
                    placeChild(split, child, childBounds, gapSize);
                }
                else {
                    break;
                }
            }
            else {
                totalProportionalUnits += child.size.desiredSpace;
            }
        }

        if (totalProportionalUnits > 0 
            && unallocatedSpace > 0) {

            auto proportionalSpace = unallocatedSpace / totalProportionalUnits;
            
            for (auto& child : children) {
                if (child.size.unit == Unit::Proportional) {
                    child.size.actualSpace = proportionalSpace * child.size.desiredSpace;
                    placeChild(split, child, childBounds, gapSize);
                }
            }
        }
    }

    uint32_t Container::getChildrenCount() {
        return children.size();
    }

    std::optional<Tile*> Container::findTile(uint32_t taskId) {
        for (auto& child : children) {
            if (std::holds_alternative<Tile>(child.child)) {
                auto& tile = std::get<Tile>(child.child);

                if (tile.handle.taskId == taskId) {
                    return &tile;
                }
            }
            else {
                auto container = std::get<Container*>(child.child);

                if (auto tile = container->findTile(taskId)) {
                    return tile;
                }
            }
        }

        return {};
    }

    void Container::composite(uint32_t volatile* frameBuffer, uint32_t displayWidth) {
        for (auto& child : children) {
           if (std::holds_alternative<Tile>(child.child)) {
                auto& tile = std::get<Tile>(child.child); 
                renderTile(frameBuffer, tile, displayWidth, {0, 0, tile.bounds.width, tile.bounds.height});
           }
           else {
                auto container = std::get<Container*>(child.child); 
                container->composite(frameBuffer, displayWidth);
           }
        }
    }

    void Container::dispatchRenderMessages() {
       for (auto& child : children) {
           if (std::holds_alternative<Tile>(child.child)) {
                auto& tile = std::get<Tile>(child.child); 

                if (tile.canRender) {
                    tile.canRender = false;
                    Render render;
                    render.recipientId = tile.handle.taskId;
                    send(IPC::RecipientType::TaskId, &render);
                }
           }
           else {
                auto container = std::get<Container*>(child.child); 
                container->dispatchRenderMessages();
           }
        } 
    }

    bool Container::focusPreviousTile() {
        auto previous {activeTaskId};

        bool isFirst {true};
        for (auto& child : children) {
            if (std::holds_alternative<Tile>(child.child)) {
                auto& tile = std::get<Tile>(child.child); 

                if (!child.focusable) {
                    continue;
                }

                if (tile.handle.taskId == activeTaskId) {
                    if (isFirst) {
                        return false;
                    }
                    else {
                        break;
                    }
                }
                else {
                    previous = tile.handle.taskId;
                }

                if (isFirst) {
                    isFirst = false;
                }
            }
        }

        activeTaskId = previous;
        return true;
    }

    bool Container::focusNextTile() {
        auto next {activeTaskId};

        for (auto it = rbegin(children); it != rend(children); ++it) {
            if (std::holds_alternative<Tile>(it->child)) {
                auto& tile = std::get<Tile>(it->child); 

                if (!it->focusable) {
                    continue;
                }

                if (tile.handle.taskId == activeTaskId) {
                    break;
                }
                else {
                    next = tile.handle.taskId;
                }
            }
        }

        activeTaskId = next;
    }

    Display::Display(Bounds screenBounds)
        : screenBounds {screenBounds} {
        root = new Container;
        root->split = Split::Horizontal;
        activeContainer = root;
        root->bounds = screenBounds;
    }

    void Display::addTile(Tile tile, Size size, bool focusable) {
        activeContainer->addChild(tile, size, focusable);
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
        auto container = new Container();
        container->split = split;
        activeContainer->addChild(container, {});
        activeContainer = container;
    }

    void Display::render() {
        root->dispatchRenderMessages();
    }

    void Display::focusPreviousTile() {
        auto container = activeContainer;

        while (container != nullptr) {
            if (!activeContainer->focusPreviousTile()) {
                container = activeContainer->parent;

                if (container != nullptr) {
                    activeContainer = container;
                }
            }
        }
    }

    void Display::focusNextTile() {
        activeContainer->focusNextTile();
    }

    void Display::changeSplitDirection(Split split) {
        activeContainer->split = split;
        activeContainer->layoutChildren();
    }

    uint32_t Display::getActiveTaskId() const {
        return activeContainer->activeTaskId;
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

    void sendActiveAppToTaskbar(uint32_t taskId, uint32_t taskbarId) {
        char path[256];
        memset(path, '\0', 256);
        sprintf(path, "/applications/%d/setActiveApp", taskbarId);

        auto result = openSynchronous(path);

        if (result.success) {
            auto readResult = readSynchronous(result.fileDescriptor, 0);
            Vostok::ArgBuffer args {readResult.buffer, sizeof(readResult.buffer)};
            args.readType();
            args.write(taskId, Vostok::ArgTypes::Uint32);

            writeSynchronous(result.fileDescriptor, readResult.buffer, sizeof(readResult.buffer));
        }
    }

    bool sendAppNameToTaskbar(const char* name, uint32_t taskbarId, uint32_t taskId) {
        char path[256];
        memset(path, '\0', 256);

        auto words = split({name, strlen(name)}, '/');
        sprintf(path, "/applications/%d/addAppName", taskbarId);

        auto result = openSynchronous(path);

        if (result.success) {
            auto readResult = readSynchronous(result.fileDescriptor, 0);
            Vostok::ArgBuffer args {readResult.buffer, sizeof(readResult.buffer)};
            args.readType();
            auto word = words.back();
            word.remove_suffix(4);
            char temp[100];
            memset(temp, '\0', 100);
            word.copy(temp, word.length());
            args.write(temp, Vostok::ArgTypes::Cstring);
            args.write(taskId, Vostok::ArgTypes::Uint32);

            writeSynchronous(result.fileDescriptor, readResult.buffer, sizeof(readResult.buffer));

            sendActiveAppToTaskbar(taskId, taskbarId);

            return true;
        }

        return false;
    }

    uint32_t Manager::launch(const char* path) {
        auto result = openSynchronous(path);

        auto entryPointResult = readSynchronous(result.fileDescriptor, 4);
        uint32_t entryPoint {0};
        memcpy(&entryPoint, entryPointResult.buffer, 4);

        if (entryPoint > 0) {

            auto pid = run(entryPoint);

            if (taskbarTaskId > 0) {
                if (!sendAppNameToTaskbar(path, taskbarTaskId, pid)) {
                    pendingTaskbarNames.push_back({std::string{path}, pid});
                }
            }

            return pid;
        }

        return 0;
    }

    Manager::Manager(uint32_t framebufferAddress) {
        linearFrameBuffer = reinterpret_cast<uint32_t volatile*>(framebufferAddress);

        taskbarTaskId = launch("/bin/taskbar.bin");
        launch("/bin/transcript.bin");
        //capcomTaskId = launch("/bin/capcom.bin");

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
        else if (message.senderTaskId == taskbarTaskId) {
            displays[0].addTile({bounds, handle}, {Unit::Fixed, 30}, false);
        }
        else {
            displays[currentDisplay].addTile({bounds, handle});
            std::fill_n(linearFrameBuffer, screenWidth * screenHeight, 0x00'62'AF'F0);
            displays[currentDisplay].renderAll(linearFrameBuffer);
        }

        ShareMemoryRequest share;
        share.ownerAddress = reinterpret_cast<uintptr_t>(windowBuffer);
        share.sharedTaskId = message.senderTaskId;
        share.recipientIsTaskId = true;
        share.size = 800 * 600 * 4;

        send(IPC::RecipientType::ServiceRegistryMailbox, &share);
    }

    void Manager::handleUpdate(const Update& message) {
        
        Bounds dirty {message.x, message.y, message.width, message.height};
        displays[currentDisplay].composite(linearFrameBuffer, message.senderTaskId, dirty);

        if (message.senderTaskId == taskbarTaskId
            && !pendingTaskbarNames.empty()) {
            
            for (auto& app : pendingTaskbarNames) {
                if (!sendAppNameToTaskbar(app.name.data(), taskbarTaskId, app.taskId)) {
                    return;
                }
            }

            pendingTaskbarNames.clear();
        } 

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

        displays[currentDisplay].splitContainer(message.direction);
        //displays[currentDisplay].renderAll(linearFrameBuffer);
    }

    void Manager::handleLaunchProgram(const LaunchProgram& message) {
        launch("/bin/dsky.bin");
    }

    void Manager::handleHideOverlay() {
        currentDisplay = previousDisplay;
        displays[currentDisplay].renderAll(linearFrameBuffer);
    }

    void Manager::handleChangeSplitDirection(const struct ChangeSplitDirection& message) {
        displays[currentDisplay].changeSplitDirection(message.direction);
        std::fill_n(linearFrameBuffer, screenWidth * screenHeight, 0x00'62'AF'F0);
        displays[currentDisplay].renderAll(linearFrameBuffer);
    }

    void Manager::handleShareMemoryResult(const ShareMemoryResult& message) {

        for(auto& display : displays) {
            if (message.succeeded && display.enableRendering(message.sharedTaskId)) {

                break;
            }
        }
    }

    void Manager::handleKeyPress(Keyboard::KeyPress& message) {
        if (message.altPressed) {
            switch (message.key) { 
                case Keyboard::VirtualKey::Up: {

                    displays[currentDisplay].focusPreviousTile();
                    break;
                }
                case Keyboard::VirtualKey::Down: {

                    displays[currentDisplay].focusNextTile();
                    break;
                }
            }

            auto activeTaskId = displays[currentDisplay].getActiveTaskId();
            sendActiveAppToTaskbar(activeTaskId, taskbarTaskId);
        }
        else {
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
                case Keyboard::VirtualKey::F2: {
                    launch("/bin/dsky.bin");
                    break;
                }
                case Keyboard::VirtualKey::F3: {
                    SplitContainer split;
                    split.direction = Split::Vertical;
                    handleSplitContainer(split);
                    break;
                }
                default: {
                    displays[currentDisplay].injectKeypress(message);
                    break;
                }
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
                            case MessageId::ChangeSplitDirection: {
                                auto message = IPC::extractMessage<ChangeSplitDirection>(buffer);
                                handleChangeSplitDirection(message);
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