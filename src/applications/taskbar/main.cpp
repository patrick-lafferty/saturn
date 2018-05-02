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

#include "taskbar.h"
#include <services/apollo/lib/application.h>
#include "layout.h"
#include <saturn/logging.h>
#include <services/virtualFileSystem/vostok.h>

using namespace Apollo;

struct DisplayItem {

    DisplayItem(char* content, uint32_t background, uint32_t fontColour, uint32_t taskId) 
        : taskId {taskId} {
        this->content = new Apollo::Observable<char*>(content);
        this->background = new Apollo::Observable<uint32_t>(background);
        this->fontColour = new Apollo::Observable<uint32_t>(fontColour);
    }

    Apollo::Observable<char*>* content;
    Apollo::Observable<uint32_t>* background;
    Apollo::Observable<uint32_t>* fontColour;
    uint32_t taskId;
};

class TaskBar : public Application<TaskBar> {
public:

    TaskBar(uint32_t screenWidth, uint32_t screenHeight) 
        : Application(screenWidth, screenHeight) {

        if (!isValid()) {
            return;
        }

        auto binder = [&](auto binding, std::string_view name) {
            using BindingType = typename std::remove_reference<decltype(*binding)>::type::ValueType;
        };

        auto collectionBinder = [&](auto binding, std::string_view name) {
            using BindingType = typename std::remove_reference<decltype(*binding)>::type::OwnerType;

            if constexpr(std::is_same<Apollo::Elements::Grid, BindingType>::value) {
                if (name.compare("applications") == 0) {
                    binding->bindTo(applications);
                }
            }
        };
        
        if (loadLayout(TaskbarApp::layout, binder, collectionBinder)) {
        }
    }

    void addApplication(char* name, uint32_t background, uint32_t fontColour, uint32_t taskId) {
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

        auto len = strlen(name) + 1;
        char* s = new char[len];
        s[len - 1] = '\0';
        strcpy(s, name);
        applications.add(new DisplayItem(s, background, fontColour, taskId), itemBinder);

        window->layoutChildren();
        window->layoutText();
        window->render();
    }

    void handleMessage(IPC::MaximumMessageBuffer& buffer) {
        switch (buffer.messageNamespace) {
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
                        printf("[TaskBar] Unhandled WM message\n");
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
                printf("[TaskBar] Unhandled message namespace\n");
            }
        }
    }

    int getFunction(std::string_view name) override {
        if (name.compare("addAppName") == 0) {
            return static_cast<int>(FunctionId::AddAppName);
        }
        else if (name.compare("setActiveApp") == 0) {
            return static_cast<int>(FunctionId::SetActiveApp);
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
            case static_cast<uint32_t>(FunctionId::AddAppName): {
                args.writeType(Vostok::ArgTypes::Cstring);
                args.writeType(Vostok::ArgTypes::Uint32);
                args.writeType(Vostok::ArgTypes::EndArg);
                break;
            }
            case static_cast<uint32_t>(FunctionId::SetActiveApp): {
                args.writeType(Vostok::ArgTypes::Uint32);
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
            case static_cast<uint32_t>(FunctionId::AddAppName): {

                auto data = args.read<char*>(ArgTypes::Cstring);
                auto taskId = args.read<uint32_t>(ArgTypes::Uint32);

                if (!args.hasErrors()) {
                    addAppName(requesterTaskId, requestId, data, taskId);
                }
                else {
                    replyWriteSucceeded(requesterTaskId, requestId, false);
                }

                break;
            }
            case static_cast<uint32_t>(FunctionId::SetActiveApp): {

                auto taskId = args.read<uint32_t>(ArgTypes::Uint32);

                if (!args.hasErrors()) {
                    setActiveApp(requesterTaskId, requestId, taskId);
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

    const uint32_t darkBlue {0x00'00'00'20u};
    const uint32_t lightBlue {0x00'64'95'EDu};

    void addAppName(uint32_t requesterTaskId, uint32_t requestId, char* data, uint32_t taskId) {
        addApplication(data, lightBlue, darkBlue, taskId);
        Vostok::replyWriteSucceeded(requesterTaskId, requestId, true);
    }

    void setActiveApp(uint32_t requesterTaskId, uint32_t requestId, uint32_t taskId) {

        for (auto i = 0u; i < applications.size(); i++) {
            auto app = applications[i];

            if (app->taskId == currentActiveApp) {
                app->background->setValue(lightBlue);
                app->fontColour->setValue(darkBlue);
            }
            else if (app->taskId == taskId) {
                app->background->setValue(darkBlue);
                app->fontColour->setValue(lightBlue);
            }
        }

        currentActiveApp = taskId;
        Vostok::replyWriteSucceeded(requesterTaskId, requestId, true);
    }

private:

    typedef Apollo::ObservableCollection<DisplayItem*, Apollo::BindableCollection<Apollo::Elements::Grid, Apollo::Elements::Grid::Bindings>> ObservableDisplays;
    ObservableDisplays applications;
    int currentActiveApp {-1};

    enum class FunctionId {
        AddAppName,
        SetActiveApp
    };
};

int taskbar_main() {

    auto screenWidth = 800u;
    auto screenHeight = 600u;

    TaskBar taskBar {screenWidth, screenHeight};

    if (!taskBar.isValid()) {
        return 1;
    }

    taskBar.startMessageLoop();

    return 0;
}