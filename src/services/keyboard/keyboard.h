#pragma once

#include <stdint.h>
#include <ipc.h>
#include <services/ps2/ps2.h>

namespace Keyboard {
    void service();

    enum class MessageId {
        KeyEvent
    };

    struct KeyEvent : IPC::Message {
        KeyEvent() {
            messageId = static_cast<uint32_t>(MessageId::KeyEvent);
            length = sizeof(KeyEvent);
            messageNamespace = IPC::MessageNamespace::Keyboard;
        }

        PS2::PhysicalKey key;
        PS2::KeyStatus status;
    };
}