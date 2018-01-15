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
#include <services/windows/lib/window.h>
#include <services/windows/lib/application.h>
#include <services/keyboard/messages.h>
#include <algorithm>
#include <parsing>

using namespace Window;
using namespace Window::Debug;

class Dsky : public Application {
public:

    Dsky(uint32_t screenWidth, uint32_t screenHeight) 
        : Application(screenWidth, screenHeight) {

        if (!isValid()) {
            return;
        }

        promptLayout = textRenderer->layoutText("\e[38;2;255;69;0m> \e[38;2;0;191;255m", screenWidth);
    }

    void messageLoop() {
        char inputBuffer[500];
        memset(inputBuffer, '\0', 500);
        int index {0};
        Text::TextLayout currentLayout;

        drawPrompt();

        notifyReadyToRender();

        auto maxInputWidth = screenWidth - promptLayout.bounds.width;

        while (true) {

            IPC::MaximumMessageBuffer buffer;
            receive(&buffer);

            switch (buffer.messageNamespace) {
                case IPC::MessageNamespace::Keyboard: {
                    switch (static_cast<Keyboard::MessageId>(buffer.messageId)) {
                        case Keyboard::MessageId::CharacterInput: {

                            auto input = IPC::extractMessage<Keyboard::CharacterInput>(buffer);
                            inputBuffer[index] = input.character;

                            clear(cursorX, 
                                cursorY, 
                                currentLayout.bounds.width, 
                                currentLayout.bounds.height);

                            auto maxWidth = currentLayout.bounds.width;
                            currentLayout = textRenderer->layoutText(inputBuffer, maxInputWidth);

                            if (needsToScroll(currentLayout.bounds.height)) {
                                scroll(currentLayout.bounds.height);
                            }

                            maxWidth = std::max(maxWidth, currentLayout.bounds.width);
                            textRenderer->drawText(currentLayout, cursorX, cursorY);
                            //updateWindowBuffer(cursorX, cursorY, maxWidth, currentLayout.bounds.height);
                            window->markAreaDirty(cursorX, cursorY, maxWidth, currentLayout.bounds.height);

                            index++;
                            break;
                        }
                        case Keyboard::MessageId::KeyPress: {
                            auto key = IPC::extractMessage<Keyboard::KeyPress>(buffer);

                            switch (key.key) {
                                case Keyboard::VirtualKey::Enter: {

                                    auto amountToScroll = (currentLayout.lines + 1) * currentLayout.lineSpace;

                                    if (needsToScroll(amountToScroll)) {
                                        scroll(amountToScroll);
                                        cursorY = screenHeight - currentLayout.lineSpace;
                                    }
                                    else {
                                        cursorY += currentLayout.bounds.height;
                                    }

                                    currentLayout = textRenderer->layoutText("", maxInputWidth);

                                    drawPrompt(); 
                                    index = 0;
                                    memset(inputBuffer, '\0', 500);
                                    break;
                                }
                            }

                            break;
                        }
                    }
                    break;
                }
                case IPC::MessageNamespace::WindowManager: {
                    switch (static_cast<MessageId>(buffer.messageId)) {
                        case MessageId::Render: {
                            updateBackBuffer(0, 0, screenWidth, screenHeight);
                            break;
                        }
                        case MessageId::Show: {
                            updateBackBuffer(0, 0, screenWidth, screenHeight);
                            break;
                        }
                    }
                }
            }
        }
    }

private:

    void drawPrompt() {
        textRenderer->drawText(promptLayout, 0, cursorY);
        //updateWindowBuffer(0, cursorY, promptLayout.bounds.width, promptLayout.bounds.height);
        window->markAreaDirty(0, cursorY, promptLayout.bounds.width, promptLayout.bounds.height);
        cursorX = promptLayout.bounds.width;
    }

    bool needsToScroll(uint32_t spaceRequired) {
        return (cursorY + spaceRequired) >= screenHeight;
    }

    void scroll(uint32_t spaceRequired) {
        auto scroll = spaceRequired - (screenHeight - cursorY);
        auto byteCount = screenWidth * (screenHeight - scroll) * 4;
        auto frameBuffer = window->getFramebuffer();

        memcpy(frameBuffer, frameBuffer + screenWidth * (scroll), byteCount);
        clear(0, screenHeight - scroll - 1, screenWidth, scroll);
        
        //updateWindowBuffer(0, 0, screenWidth, screenHeight);
        
        window->markAreaDirty(0, 0, screenWidth, screenHeight);
        
        cursorY = screenHeight - spaceRequired - 1;
    }

    Text::TextLayout promptLayout;    
    uint32_t cursorX {0}, cursorY {0};
};

int dsky_main() {

    auto screenWidth = 800u;
    auto screenHeight = 600u;

    Dsky dsky {screenWidth, screenHeight};

    if (!dsky.isValid()) {
        return 1;
    }

    dsky.messageLoop();

    return 0;
}