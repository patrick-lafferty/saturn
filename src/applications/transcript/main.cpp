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

#include "transcript.h"
#include <services/apollo/lib/application.h>
#include <services/keyboard/messages.h>
#include "layout.h"
#include <saturn/logging.h>
#include <services/virtualFileSystem/vostok.h>
#include <saturn/parsing.h>
#include <saturn/gemini/interpreter.h>

using namespace Apollo;
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

class Transcript : public Application<Transcript> {
public:

    Transcript(uint32_t screenWidth, uint32_t screenHeight) 
        : Application(screenWidth, screenHeight) {

        if (!isValid()) {
            return;
        }

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
                if (name.compare("events") == 0) {
                    binding->bindTo(events);
                }
            }
        };
        
        if (loadLayout(TranscriptApp::layout, binder, collectionBinder)) {
            Saturn::Log::subscribe();

            setupEnvironment();
        }
    }

    DisplayItem* createEntry(char* text, uint32_t background, uint32_t fontColour) {

        auto len = strlen(text) + 1;
        char* s = new char[len];
        s[len - 1] = '\0';
        strcpy(s, text);
        return new DisplayItem(s, background, fontColour);
    }

    void addFilteredEntries() {

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

        events.clear();

        if (filtered == nullptr) {
            return;
        }

        for (auto item : filtered->items) {
            auto obj = mappedItems[item];
            events.add(obj, itemBinder);
        }

        window->layoutChildren();
        window->layoutText();
        window->render();
    }

    void addLogEntry(char* entry, uint32_t background, uint32_t fontColour) {
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

        auto len = strlen(entry) + 1;
        char* s = new char[len];
        s[len - 1] = '\0';
        strcpy(s, entry);
        events.add(new DisplayItem(s, background, fontColour), itemBinder);

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

                                filterObjects(inputBuffer);

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
                                printf("[Transcript] Unhandled key\n");
                            }
                        }

                        break;
                    }
                    default: {
                        printf("[Transcript] Unhandled Keyboard event\n");
                    }
                }
                break;
            }
            case IPC::MessageNamespace::Mouse: {
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
                        screenWidth = message.width;
                        screenHeight = message.height;

                        window->resize(screenWidth, screenHeight);
                        window->layoutChildren();
                        window->layoutText();
                        window->render();

                        break;
                    }
                    default: {
                        printf("[Transcript] Unhandled WM message\n");
                    }
                }

                break;
            }
            case IPC::MessageNamespace::ServiceRegistry: {
                break;
            }
            case IPC::MessageNamespace::VFS: {
                switch (static_cast<VirtualFileSystem::MessageId>(buffer.messageId)) {
                    case VirtualFileSystem::MessageId::OpenRequest: {
                        auto request = IPC::extractMessage<VirtualFileSystem::OpenRequest>(buffer);
                        handleOpenRequest(request);
                        break;
                    }
                    case VirtualFileSystem::MessageId::ReadRequest: {
                        auto request = IPC::extractMessage<VirtualFileSystem::ReadRequest>(buffer);
                        handleReadRequest(request);                        
                        break;
                    }
                    case VirtualFileSystem::MessageId::WriteRequest: {
                        auto request = IPC::extractMessage<VirtualFileSystem::WriteRequest>(buffer);
                        handleWriteRequest(request);
                        break;
                    }
                }
                break;
            }
            default: {
                printf("[Transcript] Unhandled message namespace\n");
            }
        }
    }

    int getFunction(std::string_view name) override {
        if (name.compare("receive") == 0) {
            return static_cast<int>(FunctionId::Receive);
        }

        return -1;
    }

    void describeFunction(uint32_t requesterTaskId, uint32_t requestId, uint32_t functionId) override {
        VirtualFileSystem::ReadResult result {};
        result.requestId = requestId;
        result.success = true;
        Vostok::ArgBuffer args{result.buffer, sizeof(result.buffer)};
        args.writeType(Vostok::ArgTypes::Function);
        
        switch(functionId) {
            case static_cast<uint32_t>(FunctionId::Receive): {
                args.writeType(Vostok::ArgTypes::Cstring);
                args.writeType(Vostok::ArgTypes::EndArg);
                break;
            }
            default: {
                result.success = false;
            }
        }

        result.recipientId = requesterTaskId;
        send(IPC::RecipientType::TaskId, &result);
    }

    void writeFunction(uint32_t requesterTaskId, uint32_t requestId, uint32_t functionId, Vostok::ArgBuffer& args) override {
        using namespace Vostok;

        auto type = args.readType();

        if (type != ArgTypes::Function) {
            replyWriteSucceeded(requesterTaskId, requestId, false);
            return;
        }

        switch(functionId) {
            case static_cast<uint32_t>(FunctionId::Receive): {

                auto data = args.read<char*>(ArgTypes::Cstring);

                if (!args.hasErrors()) {
                    receive(requesterTaskId, requestId, data);
                }
                else {
                    replyWriteSucceeded(requesterTaskId, requestId, false);
                }

                break;
            }
            default: {
                replyWriteSucceeded(requesterTaskId, requestId, false);
            }
        }
    }

    void receive(uint32_t requesterTaskId, uint32_t requestId, char* data) {
        //addLogEntry(data, 0x00FF0000, 0x0000FF00);

        auto result = Saturn::Parse::read(data);

        if (std::holds_alternative<Saturn::Parse::SExpression*>(result)) {
            auto topLevel = std::get<Saturn::Parse::SExpression*>(result);

            if (topLevel->type == Saturn::Parse::SExpType::List) {
                auto root = static_cast<Saturn::Parse::List*>(topLevel)->items[0];

                if (root->type == Saturn::Parse::SExpType::List) {
                    auto object = new Saturn::Gemini::Object(static_cast<Saturn::Parse::List*>(root)); 
                    all->items.push_back(object);
                    mappedItems[object] = createEntry(data, 0x00FF0000, 0x0000FF00);

                    if (index > 0) {
                        filterObjects(inputBuffer);
                    }
                    else {
                        filtered = all;
                        addFilteredEntries();
                    }
                }
            }
        }

        Vostok::replyWriteSucceeded(requesterTaskId, requestId, true);
    }

    Function* getEqualFunction() {
        FunctionSignature sig {{Type::Object}, Type::Bool};
        return new Function {sig, [&](List* list) {
            
            auto first = list->items[0];
            auto second = list->items[1];

            if (first->type == GeminiType::Integer
                && second->type == GeminiType::Integer) {

                auto x = static_cast<Integer*>(first);
                auto y = static_cast<Integer*>(second);

                return new Boolean (x->value == y->value);
            }

            return new Boolean(false);
        }};
    }

    Function* getGreaterThanFunction() {
        FunctionSignature sig {{Type::Object}, Type::Bool};
        return new Function {sig, [&](List* list) {
            
            auto first = list->items[0];
            auto second = list->items[1];

            if (first->type == GeminiType::Integer
                && second->type == GeminiType::Integer) {

                auto x = static_cast<Integer*>(first);
                auto y = static_cast<Integer*>(second);

                return new Boolean (x->value > y->value);
            }

            return new Boolean(false);
        }};
    }

    Function* getFilterFunction() {
        FunctionSignature sig {{Type::Object}, Type::Bool};
        return new Function {sig, [&](List* list) {

            List* result {nullptr};

            if (list->items.size() != 2) {
                return result;
            }

            if (list->items[0]->type != GeminiType::List) {
                return result;
            }

            if (list->items[1]->type != GeminiType::Function) {
                return result;
            }

            auto collection = static_cast<List*>(list->items[0]);
            auto filter = static_cast<Function*>(list->items[1]);

            result = new List();
            auto tempList = new List();
            tempList->items.push_back(nullptr);

            for (auto item : collection->items)  {
                tempList->items[0] = item;
                auto filteredItem = filter->call(tempList);

                if (std::holds_alternative<Saturn::Gemini::Object*>(filteredItem)) {
                    auto object = std::get<Saturn::Gemini::Object*>(filteredItem);

                    if (object->type == GeminiType::Boolean) {
                        if (static_cast<Boolean*>(object)->value) {
                            result->items.push_back(item);
                        }
                    }

                }
                else {
                    return (List*)nullptr;
                }
            }

            delete tempList;

            return result;
            
        }};
    }

    void setupEnvironment() {
        using namespace std::literals;

        all = new Saturn::Gemini::List();

        environment.addFunction("="sv, getEqualFunction());
        environment.addFunction(">"sv, getGreaterThanFunction());
        environment.addFunction("filter"sv, getFilterFunction());
        environment.addObject("events", all);
    }

    void filterObjects(char* lambda) {

        char code[256];
        memset(code, '\0', 256);
        sprintf(code, "(filter events (lambda e (%s)))", lambda);

        auto result = Saturn::Parse::read(code);

        if (std::holds_alternative<Saturn::Parse::SExpression*>(result)) {
            auto topLevel = std::get<Saturn::Parse::SExpression*>(result);

            if (topLevel->type == Saturn::Parse::SExpType::List) {
                auto root = static_cast<Saturn::Parse::List*>(topLevel)->items[0];
                auto value = interpret(static_cast<Saturn::Parse::List*>(root), environment);

                if (std::holds_alternative<Saturn::Gemini::Object*>(value)) {
                    auto object = std::get<Saturn::Gemini::Object*>(value);

                    if (object->type == GeminiType::List) {
                        filtered = static_cast<Saturn::Gemini::List*>(object);

                        addFilteredEntries();
                    }
                }

            }
        }
    }

private:

    typedef Apollo::ObservableCollection<DisplayItem*, Apollo::BindableCollection<Apollo::Elements::ListView, Apollo::Elements::ListView::Bindings>> ObservableDisplays;
    ObservableDisplays events;
    Observable<char*> commandLine;
    Saturn::Gemini::Environment environment;
    std::map<Saturn::Gemini::Object*, DisplayItem*> mappedItems;
    char inputBuffer[500];
    int index {0};

    Saturn::Gemini::List* all, *filtered;

    enum class FunctionId {
        Receive
    };
};

int transcript_main() {

    auto screenWidth = 800u;
    auto screenHeight = 600u;

    Transcript transcript {screenWidth, screenHeight};

    if (!transcript.isValid()) {
        return 1;
    }

    transcript.startMessageLoop();

    return 0;
}