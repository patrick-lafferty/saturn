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
    class Application {
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

                    if constexpr (hasUpdateFunction<T>(0)) {
                        static_cast<T*>(this)->update();
                    }
                }
            }
        }

    protected:

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

        Window* window;
        Text::Renderer* textRenderer;
        uint32_t screenWidth, screenHeight;

    private:

        bool finished {false};
    };

}
