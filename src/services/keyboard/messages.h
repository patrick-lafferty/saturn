#pragma once

#include <stdint.h>
#include <ipc.h>
#include <services/ps2/ps2.h>

namespace Keyboard {
    enum class MessageId {
        KeyEvent,
        KeyPress,
        RedirectToWindowManager
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

    struct KeyPress : IPC::Message {
        KeyPress() {
            messageId = static_cast<uint32_t>(MessageId::KeyPress);
            length = sizeof(KeyPress);
            messageNamespace = IPC::MessageNamespace::Keyboard;
        }

        uint8_t key;
    };

    struct RedirectToWindowManager : IPC::Message {
        RedirectToWindowManager() {
            messageId = static_cast<uint32_t>(MessageId::RedirectToWindowManager);
            length = sizeof(KeyEvent);
            messageNamespace = IPC::MessageNamespace::Keyboard;
        }
    };
}