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

namespace Apollo {

    class Window;

    namespace Text {
        class Renderer;
    }

    /*
    The base class for all graphical Saturn applications, 
    Application is responsible for setting up a window,
    handling logic to update and render its window
    */
    class Application {
    public:


        /*
        Creates a Window and TextRenderer and fills the window's
        framebuffer with a default background colour

        If startHidden is true, the constructor won't blit the
        now-default-filled framebuffer to the backbuffer,
        otherwise it will
        */
        Application(uint32_t width, uint32_t height, bool startHidden = false);

        /*
        An Application is in a valid state if it successfully
        created a Window and TextRenderer object
        */
        bool isValid();

    protected:

        /*
        Sets every pixel of the window's framebuffer to be
        the window's background colour
        */
        void clear(uint32_t x, uint32_t y, uint32_t width, uint32_t height);

        /*
        Sends a message to the window manager requesting that
        it be drawn at the given offset anytime this application's
        window is composited with the main buffer
        */
        void move(uint32_t x, uint32_t y);

        /*
        Sends a message to the window manager indicating that
        this application is ready to receive render messages
        */
        void notifyReadyToRender();

        Window* window;
        Text::Renderer* textRenderer;
        uint32_t screenWidth, screenHeight;
    };

}
