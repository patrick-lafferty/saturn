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

/*
Dsky (pronounced Diss-key) is Saturn's shell, named after the physical
interface for the Apollo Guidance Computer.
*/
#include "dsky.h"
#include <services/windows/messages.h>
#include <system_calls.h>
#include <services.h>
#include <services/windows/lib/text.h>
#include <services/windows/lib/debug.h>
#include <services/keyboard/messages.h>

using namespace Window;
using namespace Window::Debug;

bool createWindow(uint32_t address) {
    
    CreateWindow create;
    create.serviceType = Kernel::ServiceType::WindowManager;
    create.bufferAddress = address;
    
    send(IPC::RecipientType::ServiceName, &create);
    
    IPC::MaximumMessageBuffer buffer;
    receive(&buffer);

    switch (buffer.messageNamespace) {
        case IPC::MessageNamespace::WindowManager: {
            switch (static_cast<MessageId>(buffer.messageId)) {
                case MessageId::CreateWindowSucceeded: {

                    return true;
                    break;
                }
            }

            break;
        }
    }

    return false;
}

/*
auto currentLayout = &sentenceLayout;

    int i = 1;

    while (true) {
        
        char num[4];
        sprintf(num, "%d", i);

        Update update;
        update.serviceType = Kernel::ServiceType::WindowManager;

        auto layout = renderer->layoutText(num, 400);
        bool scrolled {false};

        if (i % 5 == 0) {
            currentLayout = &sentenceLayout;
        }
        else {
            currentLayout = &layout;
        }

        if (cursorY + currentLayout->bounds.height >= screenHeight) {
            auto scroll = currentLayout->lineSpace + currentLayout->bounds.height - (screenHeight - cursorY);
            auto byteCount = screenWidth * (screenHeight - scroll) * 4;
            memcpy(windowBuffer->buffer, windowBuffer->buffer + screenWidth * (scroll), byteCount);
            memset(windowBuffer->buffer + (screenHeight - scroll) * screenWidth, 0, scroll * screenWidth * 4);
            update.x = 0;
            update.y = 0;
            update.width = screenWidth;
            update.height = screenHeight;
            
            scrolled = true;
            cursorY = screenHeight - currentLayout->bounds.height - 1 - currentLayout->lineSpace;
        }

        renderer->drawText(*currentLayout, cursorX, cursorY);
        drawBox(windowBuffer->buffer, cursorX, cursorY, currentLayout->bounds.width, currentLayout->bounds.height );

        if (!scrolled) {
            update.x = cursorX;
            update.y = cursorY;
            update.width = currentLayout->bounds.width;
            update.height = currentLayout->bounds.height;
        }

        send(IPC::RecipientType::ServiceName, &update);

        cursorY += currentLayout->bounds.height;

        sleep(1000);
        i++;
    }

*/

int dsky_main() {
    auto windowBuffer = new WindowBuffer;
    auto address = reinterpret_cast<uintptr_t>(windowBuffer);

    auto screenWidth = 800u;
    auto screenHeight = 600u;

    if (!createWindow(address)) {
        delete windowBuffer;
        return 1;
    }

    auto renderer = Window::Text::createRenderer(windowBuffer->buffer);

    uint32_t cursorX = 0;    
    uint32_t cursorY = 0;

    Update update;
    update.serviceType = Kernel::ServiceType::WindowManager;
    update.x = 0;//cursorX;
    update.y = 0;//cursorY;
    update.width = 800;//layout.bounds.width;
    update.height = 600;//layout.bounds.height;
    send(IPC::RecipientType::ServiceName, &update);

    while (true) {
        IPC::MaximumMessageBuffer buffer;
        receive(&buffer);

        switch (buffer.messageNamespace) {
            case IPC::MessageNamespace::Keyboard: {
                switch (static_cast<Keyboard::MessageId>(buffer.messageId)) {
                    case Keyboard::MessageId::CharacterInput: {
                        auto keypress = IPC::extractMessage<Keyboard::KeyPress>(buffer);
                        char text[] = {(char)keypress.key, '\0'};
                        auto layout = renderer->layoutText(text, 100);

                        renderer->drawText(layout, cursorX, cursorY);

                        update.x = cursorX;
                        update.y = cursorY;
                        update.width = layout.bounds.width;
                        update.height = layout.bounds.height;
                        send(IPC::RecipientType::ServiceName, &update);

                        cursorX += layout.bounds.width;
                        break;
                    }
                    case Keyboard::MessageId::KeyPress: {

                        break;
                    }
                }
                break;
            }
        }
    }
    
}