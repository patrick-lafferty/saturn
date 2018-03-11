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
#include <services/ps2/ps2.h>
#include "virtualKeys.h"

namespace Keyboard {
    enum class MessageId {
        KeyEvent,
        KeyPress,
        CharacterInput,
        RedirectToWindowManager
    };

    struct KeyEvent : IPC::Message {
        KeyEvent() {
            messageId = static_cast<uint32_t>(MessageId::KeyEvent);
            length = sizeof(KeyEvent);
            messageNamespace = IPC::MessageNamespace::Keyboard;
        }

        PS2::PhysicalKey key;
        PS2::KeyStatus status;
    };

    struct KeyPress : IPC::Message {
        KeyPress() {
            messageId = static_cast<uint32_t>(MessageId::KeyPress);
            length = sizeof(KeyPress);
            messageNamespace = IPC::MessageNamespace::Keyboard;
        }

        VirtualKey key;
        bool altPressed;
    };

    struct CharacterInput : IPC::Message {
        CharacterInput() {
            messageId = static_cast<uint32_t>(MessageId::CharacterInput);
            length = sizeof(CharacterInput);
            messageNamespace = IPC::MessageNamespace::Keyboard;
        }

        uint8_t character;
    }; 

    struct RedirectToWindowManager : IPC::Message {
        RedirectToWindowManager() {
            messageId = static_cast<uint32_t>(MessageId::RedirectToWindowManager);
            length = sizeof(KeyEvent);
            messageNamespace = IPC::MessageNamespace::Keyboard;
        }
    };
}