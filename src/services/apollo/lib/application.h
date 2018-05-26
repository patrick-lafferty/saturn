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
#pragma once
#include <stdint.h>
#include "window.h"
#include "text.h"
#include <system_calls.h>
#include <services.h>
#include "../messages.h"
#include <algorithm>
#include <saturn/time.h>
#include <services/apollo/lib/layout.h>
#include <services/apollo/lib/renderer.h>
#include <services/virtualFileSystem/messages.h>
#include <services/virtualFileSystem/vostok.h>
#include <services/mouse/messages.h>
#include <saturn/parsing.h>

namespace Apollo {

    /*
    Helper functions that determine whether T has a function update
    taking no parameters. Used to determine at compile time whether
    to add a call to update or not.
    */
    template<class T>
    constexpr auto hasUpdateFunction(int)
        -> decltype(std::declval<T>().update(), std::true_type{}) {
        return {};
    }

    template<class T>
    constexpr std::false_type hasUpdateFunction(long) {
        return {};
    }

    /*
    The base class for all graphical Saturn applications, 
    Application is responsible for setting up a window,
    handling logic to update and render its window
    */
    template<class T>
    class Application : public Vostok::Object {
    public:

        /*
        Creates a Window and TextRenderer and fills the window's
        framebuffer with a default background colour

        If startHidden is true, the constructor won't blit the
        now-default-filled framebuffer to the backbuffer,
        otherwise it will
        */
        Application(uint32_t width, uint32_t height, bool startHidden = false) 
            : screenWidth {width}, screenHeight {height} {

            window = createWindow(width, height);

            if (window == nullptr) {
                return;
            }

            textRenderer = Text::createRenderer(window);

            if (textRenderer == nullptr) {
                return;
            }

            window->setBackgroundColour(0x00'20'20'20u);
            clear(0, 0, width, height);
            window->markAreaDirty(0, 0, width, height);

            mount();

            if (!startHidden) {
                window->blitBackBuffer();
            }
        }
        /*
        An Application is in a valid state if it successfully
        created a Window and TextRenderer object
        */
        bool isValid() {
            return window != nullptr && textRenderer != nullptr;
        }

        /*
        The main loop for all graphical applications. Updates the
        window at a fixed rate, and delegates all message handling
        to the derived application.
        */
        void startMessageLoop() {
            double time = Saturn::Time::getHighResolutionTimeSeconds(); 
            double accumulator = 0.;
            double desiredFrameTime = 1.f / 30.f;

            if constexpr (hasUpdateFunction<T>(0)) {
                while (!finished) {
                    auto currentTime = Saturn::Time::getHighResolutionTimeSeconds();
                    accumulator += (currentTime - time);
                    time = currentTime;

                    IPC::MaximumMessageBuffer buffer;

                    while (peekReceive(&buffer)) {
                        static_cast<T*>(this)->handleMessage(buffer);
                    }

                    while (accumulator >= desiredFrameTime) {
                        accumulator -= desiredFrameTime;
                        static_cast<T*>(this)->update();
                    }
                }   
            }
            else {
                while (!finished) {

                    IPC::MaximumMessageBuffer buffer;
                    receive(&buffer);

                    static_cast<T*>(this)->handleMessage(buffer);
                }
            }
        }

        virtual void readSelf(uint32_t, uint32_t) override {}
        virtual int getProperty(std::string_view) override { return -1; }
        virtual void readProperty(uint32_t, uint32_t, uint32_t) override {}
        virtual void writeProperty(uint32_t, uint32_t, uint32_t, Vostok::ArgBuffer& ) override {}
        virtual Object* getNestedObject(std::string_view) override { return nullptr; }

        virtual int getFunction(std::string_view) override { return -1;}

        virtual void readFunction(uint32_t requesterTaskId, uint32_t requestId, uint32_t functionId) override {
            describeFunction(requesterTaskId, requestId, functionId);
        }

        virtual void writeFunction(uint32_t, uint32_t, uint32_t, Vostok::ArgBuffer&) override {}
        virtual void describeFunction(uint32_t, uint32_t, uint32_t) override {}

        /*
        Default handler for when derived applications just want default
        behaviour for some message
        */
        void handleMessage(IPC::MaximumMessageBuffer& buffer) {

            switch (buffer.messageNamespace) {
                case IPC::MessageNamespace::Mouse: {
                    switch (static_cast<Mouse::MessageId>(buffer.messageId)) {
                        case Mouse::MessageId::MouseMove: {
                            break;
                        }
                        case Mouse::MessageId::ButtonPress: {
                            break;
                        }
                        case Mouse::MessageId::Scroll: {
                            auto event = IPC::extractMessage<Mouse::Scroll>(buffer);
                            window->handleMouseScroll(event);

                            break;
                        }
                    }

                    break;
                }
            }
        }

    protected:

        /*
        Create an entry in the VFS for this application,
        TODO: make Application a Vostok Object
        */
        void mount() {
            VirtualFileSystem::MountRequest request;
            const char* path = "/applications";
            memcpy(request.path, path, strlen(path) + 1);
            request.serviceType = Kernel::ServiceType::VFS;
            request.cacheable = false;
            request.writeable = true;

            send(IPC::RecipientType::ServiceName, &request);
        }

        /*
        Sets every pixel of the window's framebuffer to be
        the window's background colour
        */
        void clear(uint32_t x, uint32_t y, uint32_t width, uint32_t height) {
            auto backgroundColour = window->getBackgroundColour();
            auto frameBuffer = window->getFramebuffer();

            for (auto row = 0u; row < height; row++) {
                std::fill_n(frameBuffer + x + (y + row) * screenWidth, width, backgroundColour);
            }
        }

        /*
        Sends a message to the window manager requesting that
        it be drawn at the given offset anytime this application's
        window is composited with the main buffer
        */
        void move(uint32_t x, uint32_t y) {
            Move move;
            move.serviceType = Kernel::ServiceType::WindowManager;
            move.x = x;
            move.y = y;
            send(IPC::RecipientType::ServiceName, &move);
        }

        /*
        Sends a message to the window manager indicating that
        this application is ready to receive render messages
        */
        void notifyReadyToRender() {
            ReadyToRender ready;
            ready.serviceType = Kernel::ServiceType::WindowManager;
            send(IPC::RecipientType::ServiceName, &ready);
        }

        /*
        Shuts down the application and frees all resources
        */
        void close() {
            finished = true;
        }

        /*
        Creates a tree of UIElements from a Mercury layout file,
        and sets up all the necessary databinding
        */
        template<class BindFunc, class CollectionBindFunc>
        bool loadLayout(const char* layout, BindFunc binder, CollectionBindFunc collectionBinder) {
            using namespace Saturn::Parse;

            auto result = read(layout);

            if (std::holds_alternative<SExpression*>(result)) {
                auto topLevel = std::get<SExpression*>(result);

                if (topLevel->type == SExpType::List) {
                    auto root = static_cast<List*>(topLevel)->items[0];
                    std::optional<Apollo::Elements::Control*> initialFocus;
                    elementRenderer = new Renderer(window, textRenderer);
                    window->setRenderer(elementRenderer);

                    if (auto r = Apollo::Elements::loadLayout(root, window, binder, collectionBinder, initialFocus)) {
                        window->layoutChildren();
                        window->signalWindowReady();
                        window->layoutText(textRenderer);
                        window->render();

                        if (initialFocus) {
                            window->setInitialFocus(initialFocus);
                        }

                        return true;
                    }
                }
            }

            return false;
        }

        void handleOpenRequest(VirtualFileSystem::OpenRequest& request) {
            VirtualFileSystem::OpenResult result;
            result.requestId = request.requestId;
            result.serviceType = Kernel::ServiceType::VFS;

            auto words = split({request.path, strlen(request.path)}, '/');

            if (words.size() == 1) {
                auto functionId = getFunction(words[0]);

                if (functionId >= 0) {

                    auto id = nextDescriptorId++;
                    openDescriptors.push_back({this, static_cast<uint32_t>(functionId), Vostok::DescriptorType::Function, id});
                    result.type = VirtualFileSystem::FileDescriptorType::Vostok;
                    result.fileDescriptor = id;
                    result.success = true;
                }
            }

            send(IPC::RecipientType::ServiceName, &result);
        }

        void handleReadRequest(VirtualFileSystem::ReadRequest& request) {
            auto it = std::find_if(begin(openDescriptors), end(openDescriptors), 
                [&](const auto& desc) {
                return desc.id == request.fileDescriptor;
            });

            if (it != end(openDescriptors)) {
                it->read(request.senderTaskId, request.requestId);
            }
            else {

                VirtualFileSystem::ReadResult result {};
                result.requestId = request.requestId;
                result.success = false;
                send(IPC::RecipientType::ServiceName, &result);
            }
        }

        void handleWriteRequest(VirtualFileSystem::WriteRequest& request) {
            auto it = std::find_if(begin(openDescriptors), end(openDescriptors), 
                [&](const auto& desc) {
                return desc.id == request.fileDescriptor;
            });

            if (it != end(openDescriptors)) {
                Vostok::ArgBuffer args{request.buffer, sizeof(request.buffer)};
                it->write(request.senderTaskId, request.requestId, args);
            }
            else {
                Vostok::replyWriteSucceeded(request.senderTaskId, request.requestId, false);
            }
        }

        Window* window;
        Text::Renderer* textRenderer;
        Renderer* elementRenderer;
        uint32_t screenWidth, screenHeight;

    private:

        bool finished {false};
        std::vector<Vostok::FileDescriptor> openDescriptors;
        uint32_t nextDescriptorId {0};
    };

}
