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
#include "layout.h"
#include <services/apollo/lib/layout.h>
#include <services/apollo/lib/renderer.h>
#include "commands.h"

using namespace Apollo;
using namespace Apollo::Debug;
using namespace Apollo::Elements;
using namespace Saturn::Parse;

class CapCom : public Application<CapCom> {
public:

    CapCom(uint32_t screenWidth, uint32_t screenHeight) 
        : Application(screenWidth, screenHeight, true) {

        if (!isValid()) {
            return;
        }

        memset(inputBuffer, '\0', 500);
        
        createCategories(topLevelCommands);
        currentCategory = &topLevelCommands;

        auto result = read(layout);

        if (std::holds_alternative<SExpression*>(result)) {
            auto topLevel = std::get<SExpression*>(result);

            if (topLevel->type == SExpType::List) {
                auto root = static_cast<List*>(topLevel)->items[0];

                auto binder = [&](auto binding, std::string_view name) {
                    using BindingType = typename std::remove_reference<decltype(*binding)>::type::ValueType;

                    if constexpr(std::is_same<char*, BindingType>::value) {
                        if (name.compare("commandLine") == 0) {
                            binding->bindTo(commandLine);
                        }
                        else if (name.compare("currentCategoryName") == 0) {
                            binding->bindTo(currentCategoryName);
                        }
                    }
                };

                auto collectionBinder = [&](auto binding, std::string_view name) {
                    if (name.compare("currentCommands") == 0) {
                        binding->bindTo(currentItems);
                    }
                };

                if (auto r = loadLayout(root, window, binder, collectionBinder)) {
                    window->layoutChildren();
                    auto elementRenderer = new Renderer(window, textRenderer);
                    window->layoutText(textRenderer);
                    window->render(elementRenderer);
                    window->setRenderer(elementRenderer);

                    createDisplayItems(currentItems, currentCategory, window, currentCategoryName);

                    window->layoutText(textRenderer);
                    window->render(elementRenderer);
                }
            }
        }
    }

    void doCommand(AvailableCommands command) {
        index = 0;
        memset(inputBuffer, '\0', 500);

        HideOverlay h;
        h.serviceType = Kernel::ServiceType::WindowManager;
        send(IPC::RecipientType::ServiceName, &h);

        switch (command) {
            case AvailableCommands::Split: {
                SplitContainer split;
                split.serviceType = Kernel::ServiceType::WindowManager;
                split.direction = Split::Horizontal;
                send(IPC::RecipientType::ServiceName, &split);
                break;
            }
            case AvailableCommands::ChangeSplitHorizontal: {
                ChangeSplitDirection changeSplit;
                changeSplit.serviceType = Kernel::ServiceType::WindowManager;
                changeSplit.direction = Split::Horizontal;
                send(IPC::RecipientType::ServiceName, &changeSplit);

                break;
            }
            case AvailableCommands::ChangeSplitVertical: {
                ChangeSplitDirection changeSplit;
                changeSplit.serviceType = Kernel::ServiceType::WindowManager;
                changeSplit.direction = Split::Vertical;
                send(IPC::RecipientType::ServiceName, &changeSplit);

                break;
            }
            case AvailableCommands::Launch: {

                LaunchProgram l;
                l.serviceType = Kernel::ServiceType::WindowManager;
                send(IPC::RecipientType::ServiceName, &l);

                break;
            }
        }
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
                    doCommand(command.command);
                    return;
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

        createDisplayItems(currentItems, currentCategory, window, currentCategoryName);
    }

    void handleMessage(IPC::MaximumMessageBuffer& buffer) {

        switch (buffer.messageNamespace) {
            case IPC::MessageNamespace::Keyboard: {
                switch (static_cast<Keyboard::MessageId>(buffer.messageId)) {
                    case Keyboard::MessageId::CharacterInput: {

                        auto input = IPC::extractMessage<Keyboard::CharacterInput>(buffer);
                        inputBuffer[index] = input.character;
                        index++;

                        commandLine.setValue(inputBuffer);

                        handleInput();
                        break;
                    }
                    case Keyboard::MessageId::KeyPress: {
                        auto key = IPC::extractMessage<Keyboard::KeyPress>(buffer);

                        switch (key.key) {
                            case Keyboard::VirtualKey::Enter: {

                                index = 0;
                                memset(inputBuffer, '\0', 500);

                                break;
                            }
                            case Keyboard::VirtualKey::Backspace: {
                                if (index > 0) {
                                    index--;
                                    inputBuffer[index] = '\0';

                                    commandLine.setValue(inputBuffer);

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
                    case MessageId::Resize: {
                        //ignore since CapCom is a full screen overlay
                        break;
                    }
                    default: {
                        printf("[Capcom] Unhandled WM message\n");
                    }
                }

                break;
            }
            case IPC::MessageNamespace::ServiceRegistry: {
                break;
            }
            default: {
                printf("[Capcom] Unhandled message namespace\n");
            }
        }
        
    }

private:

    char inputBuffer[500];
    int index {0};
    ObservableDisplays currentItems;
    Observable<char*> commandLine;
    Observable<char*> currentCategoryName;

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