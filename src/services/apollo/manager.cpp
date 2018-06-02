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
#include <services/mouse/messages.h>
#include <saturn/time.h>
#include <saturn/parsing.h>
#include <services/virtualFileSystem/vostok.h>
#include <saturn/logging.h>
#include "alphablend.h"

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

        using namespace std::string_literals;
        logger = new Saturn::Log::Logger("apollo"s);

        cursorCapture = new uint32_t[400];
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

        for (int y = 0; y < 20; y++) {
            std::copy_n(linearFrameBuffer + mouseX + (y + mouseY) * screenWidth, 20, cursorCapture + y * 20);
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

    void Manager::handleLaunchProgram(const LaunchProgram& /*message*/) {
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
                default: {
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
                    displays[currentDisplay].injectMessage(message);
                    break;
                }
            }
        }
    }

    const uint32_t cursorBitmap[] = {
        0xFF'00'00'00, 0xFF'00'00'00, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0xFF'00'00'00, 0xFF'00'00'00, 0xFF'00'00'00, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0xFF'00'00'00, 0xFF'62'AF'F0, 0xFF'00'00'00, 0xFF'00'00'00, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
        0xFF'00'00'00, 0xFF'62'AF'F0, 0xFF'62'AF'F0, 0xFF'00'00'00, 0xFF'00'00'00, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0xFF'00'00'00, 0xFF'62'AF'F0, 0xFF'62'AF'F0, 0xFF'62'AF'F0, 0xFF'00'00'00, 0xFF'00'00'00, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0xFF'00'00'00, 0xFF'62'AF'F0, 0xFF'62'AF'F0, 0xFF'62'AF'F0, 0xFF'62'AF'F0, 0xFF'00'00'00, 0xFF'00'00'00, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
        0xFF'00'00'00, 0xFF'62'AF'F0, 0xFF'62'AF'F0, 0xFF'62'AF'F0, 0xFF'62'AF'F0, 0xFF'62'AF'F0, 0xFF'00'00'00, 0xFF'00'00'00, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0xFF'00'00'00, 0xFF'62'AF'F0, 0xFF'62'AF'F0, 0xFF'62'AF'F0, 0xFF'62'AF'F0, 0xFF'62'AF'F0, 0xFF'62'AF'F0, 0xFF'00'00'00, 0xFF'00'00'00, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0xFF'00'00'00, 0xFF'62'AF'F0, 0xFF'62'AF'F0, 0xFF'62'AF'F0, 0xFF'62'AF'F0, 0xFF'62'AF'F0, 0xFF'62'AF'F0, 0xFF'62'AF'F0, 0xFF'00'00'00, 0xFF'00'00'00, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0xFF'00'00'00, 0xFF'62'AF'F0, 0xFF'62'AF'F0, 0xFF'62'AF'F0, 0xFF'62'AF'F0, 0xFF'62'AF'F0, 0xFF'62'AF'F0, 0xFF'62'AF'F0, 0xFF'62'AF'F0, 0xFF'00'00'00, 0xFF'00'00'00, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0xFF'00'00'00, 0xFF'62'AF'F0, 0xFF'62'AF'F0, 0xFF'62'AF'F0, 0xFF'62'AF'F0, 0xFF'62'AF'F0, 0xFF'62'AF'F0, 0xFF'62'AF'F0, 0xFF'62'AF'F0, 0xFF'62'AF'F0, 0xFF'00'00'00, 0xFF'00'00'00, 0, 0, 0, 0, 0, 0, 0, 0,
        0xFF'00'00'00, 0xFF'62'AF'F0, 0xFF'62'AF'F0, 0xFF'62'AF'F0, 0xFF'62'AF'F0, 0xFF'62'AF'F0, 0xFF'62'AF'F0, 0xFF'62'AF'F0, 0xFF'62'AF'F0, 0xFF'00'00'00, 0xFF'00'00'00, 0xFF'00'00'00, 0, 0, 0, 0, 0, 0, 0, 0,
        0xFF'00'00'00, 0xFF'62'AF'F0, 0xFF'62'AF'F0, 0xFF'62'AF'F0, 0xFF'62'AF'F0, 0xFF'62'AF'F0, 0xFF'62'AF'F0, 0xFF'62'AF'F0, 0xFF'00'00'00, 0xFF'00'00'00, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0xFF'00'00'00, 0xFF'62'AF'F0, 0xFF'62'AF'F0, 0xFF'62'AF'F0, 0xFF'62'AF'F0, 0xFF'62'AF'F0, 0xFF'00'00'00, 0xFF'00'00'00, 0xFF'00'00'00, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0xFF'00'00'00, 0xFF'62'AF'F0, 0xFF'62'AF'F0, 0xFF'62'AF'F0, 0xFF'62'AF'F0, 0xFF'00'00'00, 0xFF'00'00'00, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0xFF'00'00'00, 0xFF'62'AF'F0, 0xFF'62'AF'F0, 0xFF'62'AF'F0, 0, 0xFF'00'00'00, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0xFF'00'00'00, 0xFF'62'AF'F0, 0xFF'62'AF'F0, 0xFF'00'00'00, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0xFF'00'00'00, 0xFF'62'AF'F0, 0xFF'62'AF'F0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0xFF'00'00'00, 0xFF'00'00'00, 0xFF'00'00'00, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0xFF'00'00'00, 0xFF'00'00'00, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
    };

    void Manager::handleMouseMove(Mouse::MouseMove& move) {

        auto width = std::min(20u, screenWidth - mouseX);
        auto height = std::min(20u, screenHeight - mouseY);

        for (auto y = 0u; y < height; y++) {
            std::copy_n(cursorCapture + y * 20, width, linearFrameBuffer + mouseX + (y + mouseY) * screenWidth);
        }

        mouseX += move.deltaX;
        mouseY -= move.deltaY;

        mouseX = std::max(0, mouseX);
        mouseY = std::max(0, mouseY);

        width = std::min(20u, screenWidth - mouseX);
        height = std::min(20u, screenHeight - mouseY);

        move.absoluteX = mouseX;
        move.absoluteY = mouseY;

        displays[currentDisplay].injectMessage(move);

        for (auto y = 0u; y < height; y++) {
            std::copy_n(linearFrameBuffer + mouseX + (y + mouseY) * screenWidth, width, cursorCapture + y * 20);

            for (auto x = 0u; x < width; x++) {
                auto source = cursorBitmap[x + y * 20];
                auto destination = linearFrameBuffer[mouseX + x + (y + mouseY) * screenWidth];
                auto alpha = source >> 24;
                linearFrameBuffer[mouseX + x + (y + mouseY) * screenWidth] = blend(source, destination, alpha);
            }
        }
    }

    void Manager::handleMouseButton(Mouse::ButtonPress& message) {
        logger->info("(mouse (%s %s))", 
            message.button == Mouse::Button::Left ? "left"
                : message.button == Mouse::Button::Middle ? "middle"
                : "right",
                
            message.state == Mouse::ButtonState::Pressed ?
                "down" : "up");

        displays[currentDisplay].injectMessage(message);
    }

    void Manager::handleMouseScroll(Mouse::Scroll& message) {
        logger->info("(mouse (scroll %d))",
            message.magnitude == Mouse::ScrollMagnitude::UpBy1 ? 1 : -1
            );

        displays[currentDisplay].injectMessage(message);
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
                                displays[currentDisplay].injectMessage(message);

                                break;
                            }
                            default: {
                                printf("[WindowManager] Unhandled keyboard message id\n");
                            }
                        }

                        break;
                    }
                    case IPC::MessageNamespace::Mouse: {
                        switch (static_cast<Mouse::MessageId>(buffer.messageId)) {
                            case Mouse::MessageId::MouseMove: {
                                auto event = IPC::extractMessage<Mouse::MouseMove>(buffer);
                                handleMouseMove(event);                                

                                break;
                            }
                            case Mouse::MessageId::ButtonPress: {
                                auto event = IPC::extractMessage<Mouse::ButtonPress>(buffer);
                                handleMouseButton(event);

                                break;
                            }
                            case Mouse::MessageId::Scroll: {
                                auto event = IPC::extractMessage<Mouse::Scroll>(buffer);
                                handleMouseScroll(event);

                                break;
                            }
                            default: {
                                break;
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