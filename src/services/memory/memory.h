#pragma once

#include <stdint.h>
#include <ipc.h>

namespace Memory {

    void service();

    enum class MessageId {
        GetPhysicalReport,
        PhysicalReport
    };  

    struct GetPhysicalReport : IPC::Message {
        GetPhysicalReport() {
            messageId = static_cast<uint32_t>(MessageId::GetPhysicalReport);
            length = sizeof(GetPhysicalReport);
            messageNamespace = IPC::MessageNamespace::Memory;
        }

    };


    struct PhysicalReport : IPC::Message {
        PhysicalReport() {
            messageId = static_cast<uint32_t>(MessageId::PhysicalReport);
            length = sizeof(PhysicalReport);
            messageNamespace = IPC::MessageNamespace::Memory;
        }

        uint32_t freeMemory;
        uint32_t totalMemory;
    };
}