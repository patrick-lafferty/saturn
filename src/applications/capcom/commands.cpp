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

#include "commands.h"
#include <string.h>
#include <services/apollo/lib/window.h>

using namespace Apollo;

Command::Command(AvailableCommands command, const char* keyLiteral, const char* nameLiteral) {
    this->command = command;

    key = new char[strlen(keyLiteral)];
    strcpy(key, keyLiteral);

    name = new char[strlen(nameLiteral)];
    strcpy(name, nameLiteral);
}

Category::Category(const char* keyLiteral, const char* nameLiteral) {
    key = new char[strlen(keyLiteral)];
    strcpy(key, keyLiteral);

    name = new char[strlen(nameLiteral)];
    strcpy(name, nameLiteral);
}

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

void createDisplayItems(ObservableDisplays& items, 
    Category* currentCategory, 
    Window* window,
    Observable<char*>& currentCategoryName) {

    items.clear();

    if (currentCategory != nullptr) {

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
                items.add(new DisplayItem{command.key, 0x00'00'00'20u}, itemBinder);
                items.add(new DisplayItem{command.name, 0x00'64'95'EDu}, itemBinder);
            }
            else {
                auto& category = std::get<Category>(value);
                items.add(new DisplayItem{category.key, 0x00'00'00'20u}, itemBinder);
                items.add(new DisplayItem{category.name, 0x00'64'95'EDu}, itemBinder);
            }
        }

        currentCategoryName.setValue(currentCategory->name);
    }

    window->layoutChildren();
    window->layoutText();
    window->render();
}
