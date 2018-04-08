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

#include <services/apollo/lib/databinding.h>
#include <services/apollo/lib/layout.h>
#include <services/apollo/lib/renderer.h>
#include <services/apollo/lib/elements/grid.h>

using namespace Apollo;
using namespace Apollo::Debug;
using namespace Apollo::Elements;
using namespace Saturn::Parse;

enum class AvailableCommands {
    Split,
    ChangeSplitHorizontal,
    ChangeSplitVertical,
    Launch
};

struct Command {
    Command(AvailableCommands command, const char* keyLiteral, const char* nameLiteral) {
        this->command = command;

        key = new char[strlen(keyLiteral)];
        strcpy(key, keyLiteral);

        name = new char[strlen(nameLiteral)];
        strcpy(name, nameLiteral);
    }

    AvailableCommands command;
    char* key;
    char* name;
};

struct Category {
    Category() = default;
    Category(const char* keyLiteral, const char* nameLiteral) {
        key = new char[strlen(keyLiteral)];
        strcpy(key, keyLiteral);

        name = new char[strlen(nameLiteral)];
        strcpy(name, nameLiteral);
    }

    char* key;
    char* name;
    std::map<char, std::variant<Category, Command>> children;
};

struct DisplayItem {

    DisplayItem(char* content, uint32_t background)
        : content {content}, background {background} {}

    Observable<char*> content;
    Observable<uint32_t> background;
};

const char* layout = R"(

(grid
    (margins (vertical 100) (horizontal 50))

    (rows
        (fixed-height 100)
        (fixed-height 100)
        (fixed-height 100)
        (fixed-height 100)
    )   

    (columns
        (fixed-width 50)
        (proportional-width 1)
        (fixed-width 50)
        (proportional-width 1)
    )

    (item-source (bind currentCommands))

    (item-template
        (label (caption (bind content))
            (background (bind background)) 
        )
    )        
)

)";

void createCategories(Category& topLevelCommands) {
    Category splitCat {"c", "change split direction"};
    splitCat.children['h'] = Command {AvailableCommands::ChangeSplitHorizontal, "h", "horizontal"};
    splitCat.children['v'] = Command {AvailableCommands::ChangeSplitVertical, "v", "vertical"};

    Category container {"c", "container"};
    container.children['s'] = Command {AvailableCommands::Split, "s", "split"};
    container.children['c'] = splitCat;

    Category launch {"l", "launch"};
    launch.children['d'] = Command {AvailableCommands::Launch, "d", "dsky"};

    topLevelCommands.children['c'] = container;
    topLevelCommands.children['l'] = launch;
}

typedef ObservableCollection<DisplayItem*, BindableCollection<Grid, Grid::Bindings>> ObservableDisplays;

void createDisplayItems(ObservableDisplays& items, Category* currentCategory, Window* window) {

    if (currentCategory != nullptr) {
        items.clear();

        auto itemBinder = [](auto& item) {
            return [&](auto binding, std::string_view name) {
                using BindingType = typename std::remove_reference<decltype(*binding)>::type::ValueType;

                if constexpr(std::is_same<char*, BindingType>::value) {
                    if (name.compare("content") == 0) {
                        binding->bindTo(item->content);
                    }
                }
                else if constexpr(std::is_same<uint32_t, BindingType>::value) {
                    if (name.compare("background") == 0) {
                        binding->bindTo(item->background);
                    }
                }
            };
        };

        for (const auto& [key, value]  : currentCategory->children) {
            if (std::holds_alternative<Command>(value)) {
                auto& command = std::get<Command>(value);
                items.add(new DisplayItem{command.key, 0x00'FF'00'00u}, itemBinder);
                items.add(new DisplayItem{command.name, 0x00'00'00'FFu}, itemBinder);
            }
            else {
                auto& category = std::get<Category>(value);
                items.add(new DisplayItem{category.key, 0x00'FF'00'00u}, itemBinder);
                items.add(new DisplayItem{category.name, 0x00'00'00'FFu}, itemBinder);
            }
        }
    }

    window->layoutChildren();
}

class CapCom : public Application<CapCom> {
public:

    CapCom(uint32_t screenWidth, uint32_t screenHeight) 
        : Application(screenWidth, screenHeight, true) {


        if (!isValid()) {
            return;
        }

        //promptLayout = textRenderer->layoutText("\e[38;2;255;69;0m> \e[38;2;0;191;255m", screenWidth, 0x00'64'95'EDu);
        /*mainBackgroundColour = 0x00'00'00'80; 
        window->setBackgroundColour(mainBackgroundColour);
        clear(0, 0, screenWidth, screenHeight);*/
        memset(inputBuffer, '\0', 500);
        /*auto currentLayout = textRenderer->layoutText("Menu", screenWidth, 0x00'64'95'EDu);
        maxInputWidth = screenWidth - promptLayout.bounds.width;
        textRenderer->drawText(currentLayout, 10, 0);
        promptY = currentLayout.bounds.height;
        textAreaBackgroundColour = 0x00'00'00'20; 
        window->setBackgroundColour(textAreaBackgroundColour);
        clear(10, promptY, screenWidth - 20, currentLayout.bounds.height);
        drawPrompt();
        commandAreaY = promptY + 100;
        */
        createCategories(topLevelCommands);
        currentCategory = &topLevelCommands;
        //drawCategory();

        auto result = read(layout);

        if (std::holds_alternative<SExpression*>(result)) {
            auto topLevel = std::get<SExpression*>(result);

            if (topLevel->type == SExpType::List) {
                auto root = static_cast<List*>(topLevel)->items[0];

                auto binder = [](auto, auto) {};
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

                    createDisplayItems(currentItems, currentCategory, window);

                    window->layoutText(textRenderer);
                    window->render(elementRenderer);
                }
            }
        }
    }

    void doCommand(AvailableCommands command) {
        index = 0;
        memset(inputBuffer, '\0', 500);

        //drawCommandLine();

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

        //drawCategory();
    }

    /*void drawCommandLine() {
        clear(10 + promptLayout.bounds.width, promptY, screenWidth - 20 - promptLayout.bounds.width, promptLayout.bounds.height);

        auto maxWidth = commandLineLayout.bounds.width;
        commandLineLayout = textRenderer->layoutText(inputBuffer, maxInputWidth, 0x00'64'95'EDu);

        maxWidth = std::max(maxWidth, commandLineLayout.bounds.width);
        textRenderer->drawText(commandLineLayout, cursorX, promptY);
        window->markAreaDirty(cursorX, promptY, maxWidth, commandLineLayout.bounds.height);
    }*/

    void handleMessage(IPC::MaximumMessageBuffer& buffer) {

        switch (buffer.messageNamespace) {
            case IPC::MessageNamespace::Keyboard: {
                switch (static_cast<Keyboard::MessageId>(buffer.messageId)) {
                    case Keyboard::MessageId::CharacterInput: {

                        auto input = IPC::extractMessage<Keyboard::CharacterInput>(buffer);
                        inputBuffer[index] = input.character;

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

    /*void drawPrompt() {
        textRenderer->drawText(promptLayout, 10, promptY);
        cursorX = promptLayout.bounds.width + 10;
    }
    */

    /*void drawCategory() {
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
                    auto length = strlen(category.name);
                    index += length;
                    commandText[index] = ' ';
                    index++;
                }
            }

            auto layout = textRenderer->layoutText(commandText, screenWidth - 20, 0x00'64'95'EDu);
            textRenderer->drawText(layout, 10, commandAreaY);
            window->markAreaDirty(10, commandAreaY, screenWidth - 20, layout.bounds.height);
        }
        else {
            window->markAreaDirty(10, commandAreaY, screenWidth - 20, 300);
        }
    }*/

    /*Text::TextLayout promptLayout;    
    Text::TextLayout commandLineLayout;
    uint32_t cursorX {0}, cursorY {0};*/
    char inputBuffer[500];
    int index {0};
    /*uint32_t maxInputWidth;
    uint32_t mainBackgroundColour;
    uint32_t textAreaBackgroundColour;
    uint32_t promptY;
    uint32_t commandAreaY;*/
    ObservableDisplays currentItems;

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