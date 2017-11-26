#pragma once

#include <stdint.h>
#include <ipc.h>
#include <services/ps2/ps2.h>

namespace Keyboard {
    void service();

    struct KeyEvent : IPC::Message {
        KeyEvent() {
            messageId = MessageId;
            length = sizeof(KeyEvent);
        }

        static uint32_t MessageId;
        PS2::PhysicalKey key;
        PS2::KeyStatus status;
    };
}