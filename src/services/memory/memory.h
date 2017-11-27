#pragma once

#include <stdint.h>
#include <ipc.h>

namespace Memory {

    void service();

    struct GetPhysicalReport : IPC::Message {
        GetPhysicalReport() {
            messageId = MessageId;
            length = sizeof(GetPhysicalReport);
        }

        static uint32_t MessageId;
    };


    struct PhysicalReport : IPC::Message {
        PhysicalReport() {
            messageId = MessageId;
            length = sizeof(PhysicalReport);
        }

        static uint32_t MessageId;
        uint32_t freeMemory;
        uint32_t totalMemory;
    };
}