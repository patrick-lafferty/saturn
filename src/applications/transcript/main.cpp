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
#include "layout.h"
#include <saturn/logging.h>
#include <services/virtualFileSystem/vostok.h>

using namespace Apollo;

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
        }
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

    void readFunction(uint32_t requesterTaskId, uint32_t requestId, uint32_t functionId) override {
        describeFunction(requesterTaskId, requestId, functionId);
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
        addLogEntry(data, 0x00FF0000, 0x0000FF00);
        Vostok::replyWriteSucceeded(requesterTaskId, requestId, true);
    }

private:

    typedef Apollo::ObservableCollection<DisplayItem*, Apollo::BindableCollection<Apollo::Elements::ListView, Apollo::Elements::ListView::Bindings>> ObservableDisplays;
    ObservableDisplays events;

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