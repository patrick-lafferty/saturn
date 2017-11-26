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
        char buffer[128];
        uint32_t stringLength {0};
    };

    struct DirtyRect {
        uint32_t startIndex {0};
        uint32_t endIndex {0};
    };

    enum class CSIFinalByte {
        CursorPosition = 'H',
        SelectGraphicRendition = 'm'
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
        uint32_t handleSelectGraphicRendition(char* buffer, uint32_t length);

        uint16_t* buffer;
        uint32_t row {0}, column{0};
        uint8_t currentColour;
        DirtyRect dirty;
    };
}



