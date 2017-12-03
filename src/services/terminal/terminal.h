#pragma once

#include <stdint.h>
#include "vga.h"

namespace Terminal {
    void service();

    struct PrintMessage : IPC::Message {
        PrintMessage() {
            messageId = MessageId;
            length = sizeof(PrintMessage);
        }

        static uint32_t MessageId;
        char buffer[256];
        uint32_t stringLength {0};
    };

    struct Print32Message : IPC::Message {
        Print32Message() {
            messageId = MessageId;
            length = sizeof(Print32Message);
        }

        static uint32_t MessageId;
        char buffer[32];
        uint32_t stringLength {0};
    };

    struct Print64Message : IPC::Message {
        Print64Message() {
            messageId = MessageId;
            length = sizeof(Print64Message);
        }

        static uint32_t MessageId;
        char buffer[64];
        uint32_t stringLength {0};
    };

    struct Print128Message : IPC::Message {
        Print128Message() {
            messageId = MessageId;
            length = sizeof(Print128Message);
        }

        static uint32_t MessageId;
        char buffer[128];
        uint32_t stringLength {0};
    };

    struct KeyPress : IPC::Message {
        KeyPress() {
            messageId = MessageId;
            length = sizeof(KeyPress);
        }

        static uint32_t MessageId;
        uint8_t key;
    };

    struct GetCharacter : IPC::Message {
        GetCharacter() {
            messageId = MessageId;
            length = sizeof(GetCharacter);
        }

        static uint32_t MessageId;
    };

    struct CharacterInput : IPC::Message {
        CharacterInput() {
            messageId = MessageId;
            length = sizeof(CharacterInput);
        }

        static uint32_t MessageId;
        uint8_t character;
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

    class Terminal {
    public:

        Terminal(uint16_t* buffer);
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



