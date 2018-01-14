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

#include <stdint.h>
#include <ipc.h>

namespace Window {
    enum class MessageId {
        CreateWindow,
        CreateWindowSucceeded,
        Render,
        Update,
        Show,
        Move
    };

    struct CreateWindow : IPC::Message {
        CreateWindow() {
            messageId = static_cast<uint32_t>(MessageId::CreateWindow);
            length = sizeof(CreateWindow);
            messageNamespace = IPC::MessageNamespace::WindowManager;
        }

        uint32_t bufferAddress;
        uint32_t width;
        uint32_t height;
    };

    struct CreateWindowSucceeded : IPC::Message {
        CreateWindowSucceeded() {
            messageId = static_cast<uint32_t>(MessageId::CreateWindowSucceeded);
            length = sizeof(CreateWindowSucceeded);
            messageNamespace = IPC::MessageNamespace::WindowManager;
        }
    };

    struct Render : IPC::Message { 
        Render() {
            messageId = static_cast<uint32_t>(MessageId::Render);
            length = sizeof(Render);
            messageNamespace = IPC::MessageNamespace::WindowManager;
        }
    };

    struct Update : IPC::Message {
        Update() {
            messageId = static_cast<uint32_t>(MessageId::Update);
            length = sizeof(Update);
            messageNamespace = IPC::MessageNamespace::WindowManager;
        }

        uint32_t x, y;
        uint32_t width, height;
    };

    struct Show : IPC::Message {
        Show() {
            messageId = static_cast<uint32_t>(MessageId::Show);
            length = sizeof(Show);
            messageNamespace = IPC::MessageNamespace::WindowManager;
        }
    };

    struct Move : IPC::Message {
        Move() {
            messageId = static_cast<uint32_t>(MessageId::Move);
            length = sizeof(Move);
            messageNamespace = IPC::MessageNamespace::WindowManager;
        }

        uint32_t x, y;
    };
}