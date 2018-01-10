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

using namespace Window;

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

void drawBox(uint32_t* buffer, uint32_t positionX, uint32_t positionY, uint32_t width, uint32_t height) {
    for (auto y = positionY; y < positionY + height; y++) {

        for (auto x = positionX; x < positionX + width; x++) {
            buffer[x + y * 800] = 0x00'ff'00'00;

            if (y > positionY && y < (positionY + height - 1)) {
                x += width - 2;
            }
        }
    }
}

int dsky_main() {
    auto windowBuffer = new WindowBuffer;
    auto address = reinterpret_cast<uintptr_t>(windowBuffer);

    if (!createWindow(address)) {
        delete windowBuffer;
        return 1;
    }

    auto renderer = Window::Text::createRenderer(windowBuffer->buffer);
    char* text = "abcdefghijkl";
    auto layout = renderer->layoutText(text, 100);

    uint32_t cursorX = 0;    
    uint32_t cursorY = 0;

    while (true) {

        renderer->drawText(layout, cursorX, cursorY);

        Update update;
        update.serviceType = Kernel::ServiceType::WindowManager;
        update.x = cursorX;
        update.y = cursorY;
        update.width = layout.bounds.width;
        update.height = layout.bounds.height + layout.lineSpace; 
        
        drawBox(windowBuffer->buffer, cursorX, cursorY, layout.bounds.width, layout.bounds.height );

        send(IPC::RecipientType::ServiceName, &update);
        cursorX += 50;
        cursorY += layout.bounds.height + layout.lineSpace;

        if (cursorY + layout.bounds.height >= 600) {
            cursorY = 0;
        }

        sleep(2000);
    }
}