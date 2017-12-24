#pragma once

#include <stdint.h>
#include <ipc.h>

namespace VGA {

    void service();

    enum class MessageId {
        Blit,
        Scroll
    };

    struct BlitMessage : IPC::Message {
        BlitMessage() {
            messageId = static_cast<uint32_t>(MessageId::Blit);
            length = sizeof(BlitMessage);
            messageNamespace = IPC::MessageNamespace::VGA;
        }

        uint16_t buffer[64];
        uint32_t index {0};
        uint32_t count {0};
    };

    struct ScrollScreen : IPC::Message {
        ScrollScreen() {
            messageId = static_cast<uint32_t>(MessageId::Scroll);
            length = sizeof(ScrollScreen);
            messageNamespace = IPC::MessageNamespace::VGA;
        }

        uint32_t linesToScroll {0};
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
    void setBackgroundColour(uint8_t& colour, uint8_t backgroundColour);
    uint16_t prepareCharacter(uint8_t character, uint8_t colour);
    void disableCursor();
}