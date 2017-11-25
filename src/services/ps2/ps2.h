#pragma once

#include <stdint.h>
#include <ipc.h>

namespace PS2 {
    void service();

    struct KeyboardInput : IPC::Message {
        KeyboardInput() {
            messageId = MessageId;
            length = sizeof(KeyboardInput);
        }        

        static uint32_t MessageId;
        uint8_t data;
    };

    struct MouseInput : IPC::Message {
        MouseInput() {
            messageId = MessageId;
            length = sizeof(MouseInput);
        }

        static uint32_t MessageId;
        uint8_t data;
    };
}