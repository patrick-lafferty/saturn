#pragma once

#include <stdint.h>
#include <ipc.h>

namespace VGA {

    void service();

    struct PrintMessage : IPC::Message {
        PrintMessage() {
            messageId = MessageId;
            length = sizeof(PrintMessage);
        }

        static uint32_t MessageId;
        char buffer[128];
    };

    enum class Colours {
        Black = 0,
        DarkBlue,
        DarkGreen,
        DarkCyan,
        DarkRed,
        DarkMagenta,
        Brown,
        LightGray,
        DarkGray,
        LightBlue,
        LightGreen,
        LightCyan,
        LightRed,
        LightMagenta,
        Yellow,
        White
    };

    const int32_t Width = 80;
    const int32_t Height = 25;

    uint8_t getColour(Colours foreground, Colours background);
    uint16_t prepareCharacter(uint8_t character, uint8_t colour);
}