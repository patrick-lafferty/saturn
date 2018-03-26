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

using namespace Apollo;
using namespace Apollo::Debug;
using namespace Saturn::Parse;

void visit(SExpression*);
void visit(IntLiteral& i) {

}

void visit(Symbol& s) {

}

void visit(List& s) {
    for (auto i : s.items) {
        visit(i);
    }
}

void visit(SExpression* s) {
    switch (s->type) {
        case SExpType::IntLiteral: {
            visit(*static_cast<IntLiteral*>(s));
            break;
        }
        case SExpType::Symbol: {
            visit(*static_cast<Symbol*>(s));
            break;
        }
        case SExpType::List: {
            visit(*static_cast<List*>(s));
            break;
        }
    }
}

class TextBox {
public:

    enum class Bindings {
        Text
    };

    Bindable<TextBox, Bindings, char*> text;

    TextBox()
        : text{this, Bindings::Text} {
    }

    void onChange(Bindings b) {
        switch (b) {
            case Bindings::Text: {
                break;
            }
        }
     }
};

class Grid {
public:
    enum class Bindings {
        Items
    };

    Bindable<Grid, Bindings, std::vector<int>> items;

    Grid()
        : items{this, Bindings::Items} {}

    void onChange(Bindings b) {
        switch (b) {
            case Bindings::Items: {
                break;
            }
        }
     }
};

template<class T>
void createUI(T createBinding) {
    TextBox t;

    createBinding(&t.text, "commandLine");

    Grid g;
    createBinding(&g.items, "availableCommands");
}

void test() {
    Observable<char*> a;
    Observable<int> b;
    ObservableCollection<std::vector<int>> c;

    createUI([&](auto binding, std::string_view name) {
        using BindingType = typename std::remove_reference<decltype(*binding)>::type::ValueType;

        if constexpr(std::is_same<char*, BindingType>::value) {
            if (name.compare("commandLine")) {
                binding->bindTo(a);
            }

        }
        else if constexpr(std::is_same<int, BindingType>::value) {

            if (name.compare("availableCommands")) {
                binding->bindTo(c);
            }
        }
    });
}

class Dsky : public Application<Dsky> {
public:

    Dsky(uint32_t screenWidth, uint32_t screenHeight) 
        : Application(screenWidth, screenHeight) {

        if (!isValid()) {
            return;
        }

        promptLayout = textRenderer->layoutText("\e[38;2;255;69;0m> \e[38;2;0;191;255m", screenWidth);
        memset(inputBuffer, '\0', 500);
        drawPrompt(); 
        maxInputWidth = screenWidth - promptLayout.bounds.width;
test();
        const char* data = R"(
(grid
    (margin 5)

    (children 
        (label "Menu")
        (textbox )
        (grid 
            (auto-columns 1 1)
            (column-gap 3))))
        )";

        auto result = read(data);

        if (std::holds_alternative<SExpression*>(result)) {
            auto topLevel = std::get<SExpression*>(result);

            visit(topLevel);
        }
    }

    void drawInput() {
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
        window->markAreaDirty(cursorX, cursorY, maxWidth, currentLayout.bounds.height);
    }

    void handleMessage(IPC::MaximumMessageBuffer& buffer) {
        switch (buffer.messageNamespace) {
            case IPC::MessageNamespace::Keyboard: {
                switch (static_cast<Keyboard::MessageId>(buffer.messageId)) {
                    case Keyboard::MessageId::CharacterInput: {

                        auto input = IPC::extractMessage<Keyboard::CharacterInput>(buffer);
                        inputBuffer[index] = input.character;

                        drawInput(); 

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
                        maxInputWidth = message.width - promptLayout.bounds.width;
                        drawInput();
                        //window->resize(screenWidth, screenHeight);

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

    int x {50};
    int y {0};

    /*void update() {
        clear(x, y, 50, 50);
        x++;

        drawBox(window->getFramebuffer(), x, y, 50, 50);
        window->markAreaDirty(x, y, 50, 50);
    }*/

private:

    void drawPrompt() {
        textRenderer->drawText(promptLayout, 0, cursorY);
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
        
        window->markAreaDirty(0, 0, screenWidth, screenHeight);
        
        cursorY = screenHeight - spaceRequired - 1;
    }

    Text::TextLayout promptLayout;    
    uint32_t cursorX {0}, cursorY {0};
    char inputBuffer[500];
    int index {0};
    Text::TextLayout currentLayout;
    uint32_t maxInputWidth;
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