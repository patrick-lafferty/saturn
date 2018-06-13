/*
Copyright (c) 2017, Patrick Lafferty
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
#include "vga.h"

namespace Terminal {
    void service();

    enum class MessageId {
        Print,
        Print32,
        Print64,
        Print128,
        GetCharacter,
        CharacterInput
    };

    struct PrintMessage : IPC::Message {
        PrintMessage() {
            messageId = static_cast<uint32_t>(MessageId::Print);
            length = sizeof(PrintMessage);
            messageNamespace = IPC::MessageNamespace::Terminal;
        }

        char buffer[256];
        uint32_t stringLength {0};
    };

    struct Print32Message : IPC::Message {
        Print32Message() {
            messageId = static_cast<uint32_t>(MessageId::Print32);
            length = sizeof(Print32Message);
            messageNamespace = IPC::MessageNamespace::Terminal;
        }

        char buffer[32];
        uint32_t stringLength {0};
    };

    struct Print64Message : IPC::Message {
        Print64Message() {
            messageId = static_cast<uint32_t>(MessageId::Print64);
            length = sizeof(Print64Message);
            messageNamespace = IPC::MessageNamespace::Terminal;
        }

        char buffer[64];
        uint32_t stringLength {0};
    };

    struct Print128Message : IPC::Message {
        Print128Message() {
            messageId = static_cast<uint32_t>(MessageId::Print128);
            length = sizeof(Print128Message);
            messageNamespace = IPC::MessageNamespace::Terminal;
        }

        char buffer[128];
        uint32_t stringLength {0};
    };

    struct GetCharacter : IPC::Message {
        GetCharacter() {
            messageId = static_cast<uint32_t>(MessageId::GetCharacter);
            length = sizeof(GetCharacter);
            messageNamespace = IPC::MessageNamespace::Terminal;
        }

    };

    struct DirtyRect {
        uint32_t startIndex {0};
        uint32_t endIndex {0};
        bool overflowed {false};
        uint32_t linesOverflowed {0};
    };

    enum class CSIFinalByte {
        CursorPosition = 'H',
        CursorHorizontalAbsolute = 'G',
        SelectGraphicRendition = 'm',
    };

    class [[deprecated]] Terminal {
    public:

        explicit Terminal(uint16_t* buffer);
        void writeCharacter(uint8_t character);
        void writeCharacter(uint8_t character, uint8_t colour);
        DirtyRect interpret(char* buffer, uint32_t count);

        static Terminal& getInstance() {
            static Terminal instance{reinterpret_cast<uint16_t*>(0xD00B8000)};
            return instance;
        }

        uint32_t getIndex();
        uint16_t* getBuffer();

    private:

        uint32_t handleEscapeSequence(char* buffer, uint32_t count); 
        uint32_t handleCursorPosition(char* buffer, uint32_t length);
        uint32_t handleCursorHorizontalAbsolute(char* buffer, uint32_t length);
        uint32_t handleSelectGraphicRendition(char* buffer, uint32_t length);

        uint16_t* buffer;
        uint32_t row {0}, column{0};
        uint8_t currentColour;
        DirtyRect dirty;
    };
}



