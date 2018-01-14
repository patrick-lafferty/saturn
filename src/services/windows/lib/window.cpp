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
#include "window.h"
#include <system_calls.h>
#include "../messages.h"
#include <services.h>

namespace Window {

    Window* createWindow(uint32_t width, uint32_t height) {
        auto windowBuffer = new WindowBuffer;
        auto address = reinterpret_cast<uintptr_t>(windowBuffer);

        CreateWindow create;
        create.serviceType = Kernel::ServiceType::WindowManager;
        create.bufferAddress = address;
        create.width = width;
        create.height = height;
        
        send(IPC::RecipientType::ServiceName, &create);
        
        IPC::MaximumMessageBuffer buffer;
        filteredReceive(&buffer, IPC::MessageNamespace::WindowManager, static_cast<uint32_t>(MessageId::CreateWindowSucceeded));

        return new Window(windowBuffer, width, height);
    }

    Window::Window(WindowBuffer* buffer, uint32_t width, uint32_t height)
        : backBuffer {buffer}, width {width}, height {height} {
        this->buffer = new WindowBuffer;
        memset(this->buffer->buffer, backgroundColour, width * height * 4);
    }

    uint32_t* Window::getFramebuffer() {
        return buffer->buffer;
    }

    uint32_t Window::getBackgroundColour() {
        return backgroundColour;
    }

    void Window::setBackgroundColour(uint32_t colour) {
        backgroundColour = colour;
    }

    uint32_t Window::getWidth() {
        return width;
    }

    void Window::blitBackBuffer() {
        memcpy(backBuffer, buffer, width * height * 4);
    }
}