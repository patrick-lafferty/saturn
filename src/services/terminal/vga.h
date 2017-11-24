#pragma once

#include <stdint.h>
#include <ipc.h>

namespace VGA {

    void service();

    struct BlitMessage : IPC::Message {
        BlitMessage() {
            messageId = MessageId;
            length = sizeof(BlitMessage);
        }

        static uint32_t MessageId;
        uint16_t buffer[64];
        uint32_t index {0};
        uint32_t count {0};
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
    void setForegroundColour(uint8_t& colour, uint8_t foregroundColour);
    uint16_t prepareCharacter(uint8_t character, uint8_t colour);
}