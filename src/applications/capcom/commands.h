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

#pragma once

#include <map>
#include <variant>
#include <services/apollo/lib/databinding.h>
#include <services/apollo/lib/elements/grid.h>

enum class AvailableCommands {
    Split,
    ChangeSplitHorizontal,
    ChangeSplitVertical,
    Launch
};

struct Command {
    Command(AvailableCommands command, const char* keyLiteral, const char* nameLiteral);

    AvailableCommands command;
    char* key;
    char* name;
};

struct Category {
    Category() = default;
    Category(const char* keyLiteral, const char* nameLiteral);

    char* key;
    char* name;
    std::map<char, std::variant<Category, Command>> children;
};

struct DisplayItem {

    DisplayItem(char* content, uint32_t background, uint32_t fontColour)
        : content {content}, background {background}, fontColour {fontColour} {}

    Apollo::Observable<char*> content;
    Apollo::Observable<uint32_t> background;
    Apollo::Observable<uint32_t> fontColour;
};

void createCategories(Category& topLevelCommands);

typedef Apollo::ObservableCollection<DisplayItem*, Apollo::BindableCollection<Apollo::Elements::Grid, Apollo::Elements::Grid::Bindings>> ObservableDisplays;

void createDisplayItems(ObservableDisplays& items, 
    Category* currentCategory, 
    Apollo::Window* window,
    Apollo::Observable<char*>& currentCategoryName);