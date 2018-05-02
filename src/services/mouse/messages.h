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

namespace Mouse {
    enum class MessageId {
        MouseEvent,
        MouseMove,
        ButtonPress,
        Scroll
    };

    struct MouseEvent : IPC::Message {
        MouseEvent() {
            messageId = static_cast<uint32_t>(MessageId::MouseEvent);
            length = sizeof(MouseEvent);
            messageNamespace = IPC::MessageNamespace::Mouse;
        }

        uint8_t header;
        uint8_t xMovement;
        uint8_t yMovement;
        uint8_t optional;
    };

    struct MouseMove : IPC::Message {
        MouseMove() {
            messageId = static_cast<uint32_t>(MessageId::MouseMove);
            length = sizeof(MouseMove);
            messageNamespace = IPC::MessageNamespace::Mouse;
        }

        union {
            int deltaX {0};
            int absoluteX;
        };

        union {
            int deltaY {0}; 
            int absoluteY;
        };
    };

    enum class Button {
        Left,
        Middle,
        Right
    };

    enum class ButtonState {
        Pressed,
        Released
    };

    struct ButtonPress : IPC::Message {
        ButtonPress() {
            messageId = static_cast<uint32_t>(MessageId::ButtonPress);
            length = sizeof(ButtonPress);
            messageNamespace = IPC::MessageNamespace::Mouse;
        }

        Button button;
        ButtonState state;
    };

    enum class ScrollDirection {
        Vertical,
        Horizontal
    };

    enum class ScrollMagnitude {
        UpBy1,
        DownBy1
    };

    struct Scroll : IPC::Message {
        Scroll() {
            messageId = static_cast<uint32_t>(MessageId::Scroll);
            length = sizeof(Scroll);
            messageNamespace = IPC::MessageNamespace::Mouse;
        }

        ScrollDirection direction;
        ScrollMagnitude magnitude;
    };
}