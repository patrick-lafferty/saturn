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
#include <services/apollo/lib/databinding.h>
#include <services/apollo/lib/layout.h>
#include <services/apollo/lib/renderer.h>
#include "layout.h"

using namespace Apollo;
using namespace Apollo::Debug;
using namespace Saturn::Parse;

struct DisplayItem {
    DisplayItem(char* content/*, uint32_t background, uint32_t fontColour*/)
        : content {content}/*, background {background}, fontColour {fontColour}*/ {}

    Apollo::Observable<char*> content;
    //Apollo::Observable<uint32_t> background;
    //Apollo::Observable<uint32_t> fontColour;
};

typedef Apollo::ObservableCollection<DisplayItem*, Apollo::BindableCollection<Apollo::Elements::ListView, Apollo::Elements::ListView::Bindings>> ObservableDisplays;

class Dsky : public Application<Dsky> {
public:

    Dsky(uint32_t screenWidth, uint32_t screenHeight) 
        : Application(screenWidth, screenHeight) {

        if (!isValid()) {
            return;
        }

        memset(inputBuffer, '\0', 500);

        auto result = read(DskyApp::layout);

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
                    }
                };

                auto collectionBinder = [&](auto binding, std::string_view name) {
                    using BindingType = typename std::remove_reference<decltype(*binding)>::type::OwnerType;

                    if constexpr(std::is_same<Apollo::Elements::ListView, BindingType>::value) {
                        if (name.compare("entries") == 0) {
                            binding->bindTo(entries);
                        }
                    }
                };

                if (auto r = Apollo::Elements::loadLayout(root, window, binder, collectionBinder)) {
                    window->layoutChildren();
                    elementRenderer = new Renderer(window, textRenderer);
                    window->layoutText(textRenderer);
                    window->render(elementRenderer);
                    window->setRenderer(elementRenderer);

                    auto itemBinder = [](auto& item) {
                        return [&](auto binding, std::string_view name) {
                            using BindingType = typename std::remove_reference<decltype(*binding)>::type::ValueType;

                            if constexpr(std::is_same<char*, BindingType>::value) {
                                /*if (name.compare("content") == 0) {
                                    binding->bindTo(item->content);
                                }*/
                            }
                        };
                    };
                }
            }
        }
    }

    void addEntry() {
        auto itemBinder = [](auto& item) {
            return [&](auto binding, std::string_view name) {
                using BindingType = typename std::remove_reference<decltype(*binding)>::type::ValueType;

                if constexpr(std::is_same<char*, BindingType>::value) {
                    if (name.compare("content") == 0) {
                        binding->bindTo(item->content);
                    }
                }
            };
        };

        auto len = strlen(inputBuffer);
        char* s = new char[len];
        strcpy(s, inputBuffer);
        entries.add(new DisplayItem(s), itemBinder);

        window->layoutChildren();
        window->layoutText();
        window->render();
    }

    void handleMessage(IPC::MaximumMessageBuffer& buffer) {
        switch (buffer.messageNamespace) {
            case IPC::MessageNamespace::Keyboard: {
                switch (static_cast<Keyboard::MessageId>(buffer.messageId)) {
                    case Keyboard::MessageId::CharacterInput: {

                        auto input = IPC::extractMessage<Keyboard::CharacterInput>(buffer);
                        inputBuffer[index] = input.character;

                        commandLine.setValue(inputBuffer);

                        index++;
                        break;
                    }
                    case Keyboard::MessageId::KeyPress: {
                        auto key = IPC::extractMessage<Keyboard::KeyPress>(buffer);

                        switch (key.key) {
                            case Keyboard::VirtualKey::Enter: {

                                addEntry();

                                index = 0;
                                memset(inputBuffer, '\0', 500);

                                commandLine.setValue(inputBuffer);

                                break;
                            }
                            default: {
                                printf("[Dsky] Unhandled key\n");
                            }
                        }

                        break;
                    }
                    default: {
                        printf("[Dsky] Unhandled Keyboard event\n");
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
                        auto message = IPC::extractMessage<Resize>(buffer);
                        //screenWidth = message.width;
                        //screenHeight = message.height;

                        break;
                    }
                    default: {
                        printf("[Dsky] Unhandled WM message\n");
                    }
                }

                break;
            }
            case IPC::MessageNamespace::ServiceRegistry: {
                break;
            }
            default: {
                printf("[Dsky] Unhandled message namespace\n");
            }
        }
    }

private:

    char inputBuffer[500];
    int index {0};

    Renderer* elementRenderer;
    ObservableDisplays entries;
    Observable<char*> commandLine;
};

int dsky_main() {

    auto screenWidth = 800u;
    auto screenHeight = 600u;

    Dsky dsky {screenWidth, screenHeight};

    if (!dsky.isValid()) {
        return 1;
    }

    dsky.startMessageLoop();

    return 0;
}