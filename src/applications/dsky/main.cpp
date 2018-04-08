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

using namespace Apollo;
using namespace Apollo::Debug;
using namespace Saturn::Parse;

struct DemoItem {
    Observable<char*> content;
};

class Dsky : public Application<Dsky> {
public:

    Dsky(uint32_t screenWidth, uint32_t screenHeight) 
        : Application(screenWidth, screenHeight) {

        if (!isValid()) {
            return;
        }

        promptLayout = textRenderer->layoutText("\e[38;2;255;69;0m> \e[38;2;0;191;255m", screenWidth, 0x00'64'95'EDu);
        memset(inputBuffer, '\0', 500);
        //drawPrompt(); 
        maxInputWidth = screenWidth - promptLayout.bounds.width;

        const char* data = R"(
(grid
    (margins (vertical 20) (horizontal 20))

    (rows 
        (proportional-height 1)
        (fixed-height 50)
        (proportional-height 2))

    (columns 
        (proportional-width 1)
        (proportional-width 1))

    (row-gap 20)
    (column-gap 10)

    (item-source (bind things))

    (item-template 
        (label (caption (bind content))
            (font-colour (rgb 0 0 0))
            (background (rgb 33 146 195))
        ))

    )
    )";

/*
work backwards from the end. 
we want to create a label with caption bounds to content from something.
we need an observable<char*> content that label::caption binds to

somewhere in grid:

template<class Item>
void instantiateItemTemplate(Item item) {
    auto binder = [&](auto binding, std::string_view name) {
        binding->bindTo(item.content);
    };

    createElement(this, ?, ?, binder);
}



--------------

BindableCollection knows its owner is a Grid specifically
so it could call instantiateItemTemplate

how does ObservableCollection trigger that?

template<class ItemType, class Owner, class Binding>
class Connector {
    void itemAdded(ItemType item) {
        bindable->notifyItemAdded(item);
    }
}

--------------------

BindableCollecton {

    template<class ItemType>
    void onItemAdded(ItemType item) {

    }

    template<class ItemType>
    void bindTo(ObservableCollection<ItemType> o) {
        o.subscribe(std::bind(&BindableCollection::onItemAdded, this, 
            std::placeholders::_1));
    }
}

*/

        auto result = read(data);

        if (std::holds_alternative<SExpression*>(result)) {
            auto topLevel = std::get<SExpression*>(result);

            if (topLevel->type == SExpType::List) {
                auto root = static_cast<List*>(topLevel)->items[0];

                auto binder = [&](auto binding, std::string_view name) {
                    using BindingType = typename std::remove_reference<decltype(*binding)>::type::ValueType;

                    if constexpr(std::is_same<char*, BindingType>::value) {
                        if (name.compare("variableName") == 0) {
                            binding->bindTo(captionTest);
                        }
                    }
                };

                auto collectionBinder = [&](auto binding, std::string_view name) {
                    //using BindingType = typename std::remove_reference<decltype(*binding)>::type::ValueType;
                    binding->bindTo(items);
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
                                if (name.compare("content") == 0) {
                                    binding->bindTo(item->content);
                                    int pause = 0;
                                }
                            }
                        };
                    };

                    items.add(new DemoItem(), itemBinder);
                    items.add(new DemoItem(), itemBinder);
                    items.add(new DemoItem(), itemBinder);

                    window->layoutChildren();

                    char* x = new char[2];
                    x[0] = 'a';
                    items[0]->content.setValue(x);

                    char* xx = new char[2];
                    xx[0] = 'b';
                    items[1]->content.setValue(xx);

                    char* xxx = new char[2];
                    xxx[0] = 'c';
                    items[2]->content.setValue(xxx);

                }
            }
        }
    }

    void drawInput() {
        clear(cursorX, 
            cursorY, 
            currentLayout.bounds.width, 
            currentLayout.bounds.height);

        auto maxWidth = currentLayout.bounds.width;
        currentLayout = textRenderer->layoutText(inputBuffer, maxInputWidth, 0x00'64'95'EDu);

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

                        //drawInput(); 
                        captionTest.setValue(inputBuffer);
                    char* xx = new char[2];
                    xx[0] = 'P';
                    items[1]->content.setValue(xx);

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

                                currentLayout = textRenderer->layoutText("", maxInputWidth, 0x00'64'95'EDu);

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

    Observable<char*> captionTest;
    Renderer* elementRenderer;
    ObservableCollection<DemoItem*, BindableCollection<Apollo::Elements::Grid, Apollo::Elements::Grid::Bindings>> items;
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