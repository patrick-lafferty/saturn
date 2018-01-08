#pragma once

#include <stdint.h>
#include <ipc.h>

namespace Window {
    enum class MessageId {
        CreateWindow,
        CreateWindowSucceeded,
        Update
    };

    struct CreateWindow : IPC::Message {
        CreateWindow() {
            messageId = static_cast<uint32_t>(MessageId::CreateWindow);
            length = sizeof(CreateWindow);
            messageNamespace = IPC::MessageNamespace::WindowManager;
        }

        uint32_t bufferAddress;

    };

    struct CreateWindowSucceeded : IPC::Message {
        CreateWindowSucceeded() {
            messageId = static_cast<uint32_t>(MessageId::CreateWindowSucceeded);
            length = sizeof(CreateWindowSucceeded);
            messageNamespace = IPC::MessageNamespace::WindowManager;
        }
    };

    struct Update : IPC::Message {
        Update() {
            messageId = static_cast<uint32_t>(MessageId::Update);
            length = sizeof(Update);
            messageNamespace = IPC::MessageNamespace::WindowManager;
        }

        uint32_t x, y;
        uint32_t width, height;
    };

    struct alignas(0x1000) WindowBuffer {
        uint32_t buffer[800 * 600];
    };
}