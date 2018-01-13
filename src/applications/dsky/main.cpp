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

    auto line1 = renderer->layoutText("This parrot is \e[38;2;220;20;60mno more.", 800, Window::Text::Style::Normal);
    renderer->drawText(line1, 0, cursorY);
    cursorY += line1.bounds.height;
    
    auto line21 = renderer->layoutText("\e[38;2;100;149;237mIt has ", 800, Window::Text::Style::Normal);
    renderer->drawText(line21, 0, cursorY);
    auto line22 = renderer->layoutText("\e[38;2;127;255;0mceased", 800, Window::Text::Style::Bold);
    renderer->drawText(line22, line21.bounds.width + 5, cursorY);
    auto line23 = renderer->layoutText("to be!", 800, Window::Text::Style::Normal, true);
    renderer->drawText(line23, line21.bounds.width + line22.bounds.width + 10, cursorY);
    cursorY += line23.bounds.height;

    auto line31 = renderer->layoutText("Its ", 800, Window::Text::Style::Normal);
    renderer->drawText(line31, 0, cursorY);
    cursorX = line31.bounds.width + 5;
    auto line32 = renderer->layoutText("\e[38;2;0;100;0mexpired", 800, Window::Text::Style::Italic);
    renderer->drawText(line32, cursorX, cursorY);
    cursorX += line32.bounds.width + 5;
    auto line33 = renderer->layoutText("and gone to", 800, Window::Text::Style::Normal);
    renderer->drawText(line33, cursorX, cursorY);
    cursorX += line33.bounds.width + 5;
    auto line34 = renderer->layoutText("meet ", 800, Window::Text::Style::Italic);
    renderer->drawText(line34, cursorX, cursorY);
    cursorX += line34.bounds.width + 5;
    auto line35 = renderer->layoutText("\e[38;2;255;215;0mits maker.", 800, Window::Text::Style::Bold);
    renderer->drawText(line35, cursorX, cursorY);
    cursorX = 0;
    cursorY += line34.bounds.height;

    auto line4 = renderer->layoutText("This is a \e[38;2;0;0;0mlate parrot!", 800, Window::Text::Style::Normal, false, 22);
    renderer->drawText(line4, 0, cursorY);
    cursorY += line4.bounds.height;

    auto line51 = renderer->layoutText("\e[38;2;100;149;237mIts a", 800, Window::Text::Style::Normal, false, 24);
    renderer->drawText(line51, 0, cursorY);
    cursorX += line51.bounds.width + 10;
    auto line52 = renderer->layoutText("\e[38;2;255;165;0mstiff!", 800, Window::Text::Style::Bold, false, 24);
    renderer->drawText(line52, cursorX, cursorY);
    cursorX += line52.bounds.width + 10;
    auto line53 = renderer->layoutText("\e[38;2;100;149;237mBereft of life", 800, Window::Text::Style::Italic, false, 24);
    renderer->drawText(line53, cursorX, cursorY);
    cursorX += line53.bounds.width + 5;
    auto line54 = renderer->layoutText(", it", 800, Window::Text::Style::Normal, false, 24);
    renderer->drawText(line54, cursorX, cursorY);
    cursorX += line54.bounds.width + 10;
    auto line55 = renderer->layoutText("\e[38;2;128;0;128mrests", 800, Window::Text::Style::Bold, true, 24);
    renderer->drawText(line55, cursorX, cursorY);
    cursorX += line55.bounds.width + 10;
    auto line56 = renderer->layoutText("in", 800, Window::Text::Style::Normal, false, 24);
    renderer->drawText(line56, cursorX, cursorY);
    cursorX += line56.bounds.width + 10;
    auto line57 = renderer->layoutText("\e[38;2;128;0;128mpeace!", 800, Window::Text::Style::Bold, true, 24);
    renderer->drawText(line57, cursorX, cursorY);
    cursorX = 0;
    cursorY += line57.bounds.height;

    auto line61 = renderer->layoutText("\e[38;2;100;149;237mIf you hadn't", 800, Window::Text::Style::Normal, false, 28);
    renderer->drawText(line61, 0, cursorY);
    cursorX += line61.bounds.width + 10;
    auto line62 = renderer->layoutText("\e[38;2;255;69;0mnailed it", 800, Window::Text::Style::Bold, false, 28);
    renderer->drawText(line62, cursorX, cursorY);
    cursorX += line62.bounds.width + 10;
    auto line63 = renderer->layoutText("\e[38;2;100;149;237mto the", 800, Window::Text::Style::Normal, false, 28);
    renderer->drawText(line63, cursorX, cursorY);
    cursorX += line63.bounds.width + 10;
    auto line64 = renderer->layoutText("perch", 800, Window::Text::Style::Italic, false, 28);
    renderer->drawText(line64, cursorX, cursorY);
    cursorX += line64.bounds.width + 10;
    auto line65 = renderer->layoutText("it'd", 800, Window::Text::Style::Normal, false, 28);
    renderer->drawText(line65, cursorX, cursorY);
    cursorX = 0;
    cursorY += line65.bounds.height + 10;
    auto line66 = renderer->layoutText("be \e[38;2;255;5;5mpushing up the daisies!", 800, Window::Text::Style::Normal, true, 28);
    renderer->drawText(line66, cursorX, cursorY);
    cursorY += line66.bounds.height;

    auto line71 = renderer->layoutText("\e[38;2;100;149;237mIts run \e[38;2;255;99;71mdown the curtain \e[38;2;100;149;237mand joined", 800, Window::Text::Style::Italic, true, 32);
    renderer->drawText(line71, 0, cursorY);
    cursorY += line71.bounds.height;
    auto line72 = renderer->layoutText("the \e[38;2;0;255;0mchoir invisible!", 800, Window::Text::Style::Bold, true, 32);
    renderer->drawText(line72, 0, cursorY);
    cursorY += line72.bounds.height;

    auto line81 = renderer->layoutText("\e[38;2;128;0;0mThis", 800, Window::Text::Style::Normal, false, 48);
    renderer->drawText(line81, 0, cursorY);
    cursorX += line81.bounds.width + 10;
    auto line82 = renderer->layoutText("\e[38;2;100;149;237mis an", 800, Window::Text::Style::Normal, false, 48);
    renderer->drawText(line82, cursorX, cursorY);
    cursorX += line82.bounds.width + 10;
    auto line83 = renderer->layoutText("\e[38;2;135;206;250mex-parrot!", 800, Window::Text::Style::Bold, true, 48);
    renderer->drawText(line83, cursorX, cursorY);

    update.x = 0;//cursorX;
    update.y = 0;//cursorY;
    update.width = 800;//layout.bounds.width;
    update.height = 600;//layout.bounds.height;
    send(IPC::RecipientType::ServiceName, &update);

    cursorY += 100;

    while (true) {
        IPC::MaximumMessageBuffer buffer;
        receive(&buffer);

        switch (buffer.messageNamespace) {
            case IPC::MessageNamespace::Keyboard: {
                switch (static_cast<Keyboard::MessageId>(buffer.messageId)) {
                    case Keyboard::MessageId::KeyPress: {
                        auto keypress = IPC::extractMessage<Keyboard::KeyPress>(buffer);
                        char text[] = {(char)keypress.key, '\0'};
                        auto layout = renderer->layoutText(text, 100);

                        renderer->drawText(layout, cursorX, cursorY);

                        update.x = 0;//cursorX;
                        update.y = 0;//cursorY;
                        update.width = 800;//layout.bounds.width;
                        update.height = 600;//layout.bounds.height;
                        send(IPC::RecipientType::ServiceName, &update);

                        cursorX += layout.bounds.width;
                    }
                }
                break;
            }
        }
    }
    
}