#pragma once

#include <stdint.h>
#include <ipc.h>

namespace Splash {

    void service();

    struct SplashScreen : IPC::Message {
        SplashScreen() {
            messageId = MessageId;
            length = sizeof(SplashScreen);
        }

        static uint32_t MessageId;
    };
}