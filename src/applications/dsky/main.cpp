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

#include <saturn/gemini/interpreter.h>
#include <services/virtualFileSystem/vostok.h>

using namespace Apollo;
using namespace Apollo::Debug;
using namespace Saturn::Parse;

using namespace Saturn::Gemini;

struct DisplayItem {

    DisplayItem(char* content, uint32_t background, uint32_t fontColour) {
        this->content = new Apollo::Observable<char*>(content);
        this->background = new Apollo::Observable<uint32_t>(background);
        this->fontColour = new Apollo::Observable<uint32_t>(fontColour);
    }

    Apollo::Observable<char*>* content;
    Apollo::Observable<uint32_t>* background;
    Apollo::Observable<uint32_t>* fontColour;
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
                    elementRenderer = new Renderer(window, textRenderer);
                    window->layoutChildren();
                    window->setRenderer(elementRenderer);
                    window->layoutText(textRenderer);
                    window->render();

                    setupEnvironment();
                }
            }
        }
    }

    Function getVersionFunc() {
        FunctionSignature sig {{}, Type::Void};
        Function f{sig, [&](List* list) {
            char* copy = new char[13];
            strcpy(copy, "Saturn 0.2.0");
            return Result {copy};
        }};

        return f;
    }

    char* listDirectory(char* path) {
        using namespace VirtualFileSystem;

        auto openResult = openSynchronous(path);

        if (!openResult.success) {
            return nullptr;
        }

        char* names = new char[256];
        int offset = 0;
        memset(names, 0, 256);

        auto readResult = readSynchronous(openResult.fileDescriptor, 0);

        if (!readResult.success) {
            return nullptr;
        }

        if (openResult.type == FileDescriptorType::Vostok) {
            Vostok::ArgBuffer args{readResult.buffer, sizeof(readResult.buffer)};

            auto type = args.readType();
            if (type == Vostok::ArgTypes::Property) {

                while (type != Vostok::ArgTypes::EndArg) {

                    type = args.peekType();

                    switch(type) {
                        case Vostok::ArgTypes::Void: {
                            return names;
                        }
                        case Vostok::ArgTypes::Uint32: {
                            auto value = args.read<uint32_t>(type);
                            if (!args.hasErrors()) {
                                auto c = sprintf(names + offset, "%d ", value);
                                offset += c;
                            }
                            break;
                        }
                        case Vostok::ArgTypes::Cstring: {
                            auto value = args.read<char*>(type);
                            if (!args.hasErrors()) {
                                strcpy(names + offset, value);
                                offset += strlen(value);
                                names[offset] = ' ';
                                offset++;
                            }
                            break;
                        }
                        case Vostok::ArgTypes::EndArg: {
                            return names;
                        }
                        default: {
                        }
                    }
                }
            }
        }

        ::close(openResult.fileDescriptor);

        return names;
    }

    Function getListFunc() {
        FunctionSignature sig {{Type::String}, Type::Void};
        Function f{sig, [&](List* list) {
            auto first = list->items[1];

            std::string_view value;

            if (first->type == SExpType::Symbol) {
                auto s = static_cast<Symbol*>(first);
                value = s->value;
            }
            else if (first->type == SExpType::StringLiteral) {
                auto s = static_cast<StringLiteral*>(first);
                value = s->value;
            }
            else {
                return Result {};
            }

            char path[256];
            memset(path, 0, 256);
            value.copy(path, value.length());

            auto result = listDirectory(path);
            return Result {result};
        }};

        return f; 
    }

    void test() {
        FunctionSignature sig {{Type::String}, Type::Void};
        Function f{sig, [&](List* list) {
            auto first = list->items[1];

            std::string_view value;

            if (first->type == SExpType::Symbol) {
                auto s = static_cast<Symbol*>(first);
                value = s->value;
            }
            else if (first->type == SExpType::StringLiteral) {
                auto s = static_cast<StringLiteral*>(first);
                value = s->value;
            }
            else {
                return Result {};
            }

            char* copy = new char[value.length() + 1];
            value.copy(copy, value.length());
            return Result {copy};

        }};
    }

    void setupEnvironment() {
        using namespace std::literals;

        environment.addFunction("version"sv, getVersionFunc());
        environment.addFunction("read"sv, getListFunc());
    }

    void handleInput() {
        auto result = read(inputBuffer);

        if (std::holds_alternative<SExpression*>(result)) {
            auto topLevel = std::get<SExpression*>(result);

            if (topLevel->type == SExpType::List) {
                auto root = static_cast<List*>(topLevel)->items[0];
                auto value = interpret(static_cast<List*>(root), environment);

                if (std::holds_alternative<Result>(value)) {
                    auto& r = std::get<Result>(value);

                    if (std::holds_alternative<char*>(r.value)) {
                        auto str = std::get<char*>(r.value);

                        if (str == nullptr) {
                            return;
                        }

                        addEntry(str, 0xFF'08'08'08, 0x00'64'95'EDu);

                        delete str;
                    }
                }
            }
        }
    }

    void addEntry(char* text, uint32_t background, uint32_t fontColour) {
        auto itemBinder = [](auto& item) {
            return [&](auto binding, std::string_view name) {
                using BindingType = typename std::remove_reference<decltype(*binding)>::type::ValueType;

                if constexpr(std::is_same<char*, BindingType>::value) {
                    if (name.compare("content") == 0) {
                        binding->bindTo(*item->content);
                    }
                }
                else if constexpr(std::is_same<uint32_t, BindingType>::value) {
                    if (name.compare("background") == 0) {
                        binding->bindTo(*item->background);
                    }
                    else if (name.compare("fontColour") == 0) {
                        binding->bindTo(*item->fontColour);
                    }
                }
            };
        };

        auto len = strlen(text) + 1;
        char* s = new char[len];
        s[len - 1] = '\0';
        strcpy(s, text);
        entries.add(new DisplayItem(s, background, fontColour), itemBinder);

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

                                if (index > 0) {
                                    addEntry(inputBuffer, 20, 0x00'64'95'EDu);
                                }

                                handleInput();

                                index = 0;
                                memset(inputBuffer, '\0', 500);

                                commandLine.setValue(inputBuffer);

                                break;
                            }
                            case Keyboard::VirtualKey::Backspace: {
                                if (index > 0) {
                                    index--;
                                    inputBuffer[index] = '\0';

                                    commandLine.setValue(inputBuffer);
                                }

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
    Saturn::Gemini::Environment environment;
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