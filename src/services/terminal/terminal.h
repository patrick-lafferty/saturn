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
    };

    class Terminal {
    public:

        Terminal(uint16_t* buffer);
        void writeCharacter(uint8_t character);
        void writeCharacter(uint8_t character, uint8_t colour);

        static Terminal& getInstance() {
            static Terminal instance{reinterpret_cast<uint16_t*>(0xD00B8000)};
            return instance;
        }

        uint32_t getIndex();
        uint16_t* getBuffer();

    private:

        uint16_t* buffer;
        uint32_t row {0}, column{0};
        uint8_t defaultColour;
    };
}



