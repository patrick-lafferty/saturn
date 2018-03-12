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
CapCom is the main interface between the window manager and the user
*/
#include "capcom.h"
#include <services/apollo/messages.h>
#include <system_calls.h>
#include <services.h>
#include <services/apollo/lib/text.h>
#include <services/apollo/lib/debug.h>
#include <services/apollo/lib/window.h>
#include <services/apollo/lib/application.h>
#include <services/keyboard/messages.h>
#include <algorithm>
#include <saturn/parsing.h>
#include <map>
#include <variant>

using namespace Apollo;
using namespace Apollo::Debug;

enum class AvailableCommands {
    Split,
    ChangeSplit
};

struct Command {
    AvailableCommands command;
    char name[30];
};

struct Category {
    char name[30];
    std::map<char, std::variant<Category, Command>> children;
};

class CapCom : public Application<CapCom> {
public:

    CapCom(uint32_t screenWidth, uint32_t screenHeight) 
        : Application(screenWidth, screenHeight, true) {


        if (!isValid()) {
            return;
        }

        promptLayout = textRenderer->layoutText("\e[38;2;255;69;0m> \e[38;2;0;191;255m", screenWidth);
        mainBackgroundColour = 0x00'00'00'80; 
        window->setBackgroundColour(mainBackgroundColour);
        clear(0, 0, screenWidth, screenHeight);
        memset(inputBuffer, '\0', 500);
        auto currentLayout = textRenderer->layoutText("Menu", screenWidth);
        maxInputWidth = screenWidth - promptLayout.bounds.width;
        textRenderer->drawText(currentLayout, 10, 0);
        promptY = currentLayout.bounds.height;
        textAreaBackgroundColour = 0x00'00'00'20; 
        window->setBackgroundColour(textAreaBackgroundColour);
        clear(10, promptY, screenWidth - 20, currentLayout.bounds.height);
        drawPrompt();
        commandAreaY = promptY + 100;

        Category container {"c => Container"};
        container.children['s'] = Command {AvailableCommands::Split, "s => split"};
        container.children['c'] = Command {AvailableCommands::ChangeSplit, "c => change split direction"};

        topLevelCommands.children['c'] = container;
        currentCategory = &topLevelCommands;
        drawCategory();
    }

    void handleInput() {

        currentCategory = &topLevelCommands;

        for (int i = 0; i < index; i++) {
            if (inputBuffer[i] == '\0') {
                break;
            }

            auto character = inputBuffer[i];

            if (auto child = currentCategory->children.find(character);
                child != end(currentCategory->children)) {
                if (std::holds_alternative<Command>(child->second)) {
                    auto& command = std::get<Command>(child->second);
                }
                else {
                    auto& category = std::get<Category>(child->second);
                    currentCategory = &category;
                }
            }
            else {
                currentCategory = nullptr;
                break;
            }
        }

        drawCategory();
    }

    void drawCommandLine() {
        clear(10 + promptLayout.bounds.width, promptY, screenWidth - 20 - promptLayout.bounds.width, promptLayout.bounds.height);

        auto maxWidth = commandLineLayout.bounds.width;
        commandLineLayout = textRenderer->layoutText(inputBuffer, maxInputWidth);

        maxWidth = std::max(maxWidth, commandLineLayout.bounds.width);
        textRenderer->drawText(commandLineLayout, cursorX, promptY);
        window->markAreaDirty(cursorX, promptY, maxWidth, commandLineLayout.bounds.height);
    }

    void handleMessage(IPC::MaximumMessageBuffer& buffer) {

        switch (buffer.messageNamespace) {
            case IPC::MessageNamespace::Keyboard: {
                switch (static_cast<Keyboard::MessageId>(buffer.messageId)) {
                    case Keyboard::MessageId::CharacterInput: {

                        auto input = IPC::extractMessage<Keyboard::CharacterInput>(buffer);
                        inputBuffer[index] = input.character;

                        drawCommandLine();
                        
                        index++;

                        handleInput();
                        break;
                    }
                    case Keyboard::MessageId::KeyPress: {
                        auto key = IPC::extractMessage<Keyboard::KeyPress>(buffer);

                        switch (key.key) {
                            case Keyboard::VirtualKey::Enter: {

                                drawPrompt(); 
                                index = 0;
                                memset(inputBuffer, '\0', 500);

                                HideOverlay h;
                                h.serviceType = Kernel::ServiceType::WindowManager;
                                send(IPC::RecipientType::ServiceName, &h);

                                LaunchProgram l;
                                l.serviceType = Kernel::ServiceType::WindowManager;
                                send(IPC::RecipientType::ServiceName, &l);

                                break;
                            }
                            case Keyboard::VirtualKey::Backspace: {
                                if (index > 0) {
                                    index--;
                                    inputBuffer[index] = '\0';

                                    drawCommandLine();
                                    handleInput();
                                }
                                break;
                            }
                            default: {
                                printf("[Capcom] Unhandled key\n");
                            }
                        }

                        break;
                    }
                    default: {
                        printf("[Capcom] Unhandled keyboard event\n");
                    }
                }
                break;
            }
            case IPC::MessageNamespace::WindowManager: {
                switch (static_cast<MessageId>(buffer.messageId)) {
                    case MessageId::Render: {
                        window->blitBackBuffer();
                        break;
                    }
                    case MessageId::Show: {
                        window->blitBackBuffer();
                        break;
                    }
                    default: {
                        printf("[Capcom] Unhandled WM message\n");
                    }
                }

                break;
            }
            default: {
                printf("[Capcom] Unhandled message namespace\n");
            }
        }
        
    }

private:

    void drawPrompt() {
        textRenderer->drawText(promptLayout, 10, promptY);
        cursorX = promptLayout.bounds.width + 10;
    }

    void drawCategory() {
        clear(10, commandAreaY, screenWidth - 20, 300);
        char commandText[1000];
        memset(commandText, 0, sizeof(commandText));
        auto index = 0;

        if (currentCategory != nullptr) {

            for (const auto& [key, value]  : currentCategory->children) {
                if (std::holds_alternative<Command>(value)) {
                    auto& command = std::get<Command>(value);
                    strcat(commandText + index, command.name);
                    auto length = strlen(command.name);
                    index += length;
                    commandText[index] = ' ';
                    index++;
                }
                else {
                    auto& category = std::get<Category>(value);
                    strcat(commandText + index, category.name);
                    index += 30;
                }
            }

            auto layout = textRenderer->layoutText(commandText, screenWidth - 20);
            textRenderer->drawText(layout, 10, commandAreaY);
            window->markAreaDirty(10, commandAreaY, screenWidth - 20, layout.bounds.height);
        }
        else {
            window->markAreaDirty(10, commandAreaY, screenWidth - 20, 300);
        }
    }

    Text::TextLayout promptLayout;    
    Text::TextLayout commandLineLayout;
    uint32_t cursorX {0}, cursorY {0};
    char inputBuffer[500];
    int index {0};
    uint32_t maxInputWidth;
    uint32_t mainBackgroundColour;
    uint32_t textAreaBackgroundColour;
    uint32_t promptY;
    uint32_t commandAreaY;

    Category topLevelCommands;
    Category* currentCategory;
};

int capcom_main() {

    auto screenWidth = 800u;
    auto screenHeight = 600u;

    CapCom capcom {screenWidth, screenHeight};

    if (!capcom.isValid()) {
        return 1;
    }

    capcom.startMessageLoop();

    return 0;
}