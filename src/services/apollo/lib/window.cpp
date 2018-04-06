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
#include <algorithm>
#include "renderer.h"

namespace Apollo {

    Window* createWindow(uint32_t width, uint32_t height) {

        CreateWindow create;
        create.serviceType = Kernel::ServiceType::WindowManager;
        create.width = width;
        create.height = height;
        
        send(IPC::RecipientType::ServiceName, &create);

        uintptr_t address {0};

        {
            IPC::MaximumMessageBuffer buffer;
            filteredReceive(&buffer, 
                IPC::MessageNamespace::ServiceRegistry, 
                static_cast<uint32_t>(Kernel::MessageId::ShareMemoryInvitation));

            auto invitation = IPC::extractMessage<Kernel::ShareMemoryInvitation>(buffer);
            address = reinterpret_cast<uintptr_t>(aligned_alloc(0x1000, invitation.size));
            Kernel::ShareMemoryResponse response;
            response.sharedAddress = address;
            response.accepted = true;
            response.recipientId = invitation.senderTaskId;
            send(IPC::RecipientType::TaskId, &response);
        }

        { 
            IPC::MaximumMessageBuffer buffer;
            filteredReceive(&buffer, 
                IPC::MessageNamespace::WindowManager, 
                static_cast<uint32_t>(MessageId::CreateWindowSucceeded));
        }

        auto windowBuffer = reinterpret_cast<WindowBuffer*>(address);

        return new Window(windowBuffer, width, height);
    }

    Window::Window(WindowBuffer* buffer, uint32_t width, uint32_t height)
        : backBuffer {buffer}, width {width}/*, height {height}*/ {
        this->buffer = new WindowBuffer;
        memset(this->buffer->buffer, backgroundColour, width * height * 4);
        dirty = false;
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

    void updateWindowBuffer(uint32_t x, uint32_t y, uint32_t width, uint32_t height) {
        Update update;
        update.serviceType = Kernel::ServiceType::WindowManager;
        update.x = x;
        update.y = y;
        update.width = width;
        update.height = height;
        send(IPC::RecipientType::ServiceName, &update);
    }

    void Window::blitBackBuffer() {
        if (dirty) {
            auto destinationBuffer = backBuffer->buffer;
            auto sourceBuffer = buffer->buffer;

            for (auto y = 0u; y < dirtyArea.height; y++) {
                y += dirtyArea.y;
                auto destination = destinationBuffer + dirtyArea.x + y * width;
                auto source = sourceBuffer + dirtyArea.x + y * width;

                memcpy(destination, source, dirtyArea.width * sizeof(uint32_t)); 
            }

            updateWindowBuffer(dirtyArea.x, dirtyArea.y, dirtyArea.width, dirtyArea.height);

            dirty = false;
            dirtyArea = {};
        }
        else {
            updateWindowBuffer(0, 0, 0, 0);
        }
    }

    void Window::markAreaDirty(uint32_t x, uint32_t y, uint32_t width, uint32_t height) {
        dirty = true;

        dirtyArea.x = std::min(x, dirtyArea.x);
        dirtyArea.y = std::min(y, dirtyArea.y);

        dirtyArea.width = std::max(dirtyArea.width + dirtyArea.x, width + x) - dirtyArea.x;
        dirtyArea.height = std::max(dirtyArea.height + dirtyArea.y, height + y) - dirtyArea.y;
    }

    void Window::resize(uint32_t width, uint32_t height) {
        this->width = width;
        //this->height = height;
    }
    
    void Window::addChild(Elements::UIElement*) {

    }

    void Window::addChild(Elements::UIElement*, const std::vector<Elements::MetaData>&) {

    }

    void Window::addChild(Elements::Container* container) {
        child = container;
        child->setParent(this);
    }

    void Window::addChild(Elements::Container* container, const std::vector<Elements::MetaData>&) {
        child = container;
        child->setParent(this);
    }

    void Window::layoutChildren() {
        if (child != nullptr) {
            child->layoutChildren();
        }
    }

    Elements::Bounds Window::getChildBounds(const Elements::UIElement* child) {
        return {0, 0, (int)width, 600};
    }

    void Window::layoutText(Apollo::Text::Renderer* renderer) {
        if (child != nullptr) {
            child->layoutText(renderer);
        }
    }

    void Window::render(Renderer* renderer) {
        if (child != nullptr) {
            child->render(renderer);
        }
    }
}