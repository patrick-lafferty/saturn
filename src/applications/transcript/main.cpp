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

using namespace Apollo;

struct DisplayItem {

    DisplayItem(char* content, uint32_t background, uint32_t fontColour)
        : content {content}, background {background}, fontColour {fontColour} {
        //this->content = new Apollo::Observable<char*>(content);
        //this->background = new Apollo::Observable<uint32_t>(background);
        //this->fontColour = new Apollo::Observable<uint32_t>(fontColour);
    }

    Apollo::Observable<char*> content;
    Apollo::Observable<uint32_t> background;
    Apollo::Observable<uint32_t> fontColour;
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

        }
    }

    void addLogEntry(char* entry, uint32_t background, uint32_t fontColour) {
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
                    else if (name.compare("fontColour") == 0) {
                        binding->bindTo(item->fontColour);
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
            default: {
                printf("[Transcript] Unhandled message namespace\n");
            }
        }
    }

private:

    typedef Apollo::ObservableCollection<DisplayItem*, Apollo::BindableCollection<Apollo::Elements::ListView, Apollo::Elements::ListView::Bindings>> ObservableDisplays;
    ObservableDisplays events;
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