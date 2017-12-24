#include "memory.h"
#include <system_calls.h>
#include <services.h>
#include <memory/physical_memory_manager.h>

using namespace Kernel;

namespace Memory {

    void handleMessage(IPC::Message* message) {

        if (message->messageNamespace == IPC::MessageNamespace::Memory
                && message->messageId == static_cast<uint32_t>(MessageId::GetPhysicalReport)) {
            PhysicalReport report {};
            report.freeMemory = Memory::currentPMM->getFreePages();
            report.totalMemory = Memory::currentPMM->getTotalPages();
            report.recipientId = message->senderTaskId;

            send(IPC::RecipientType::TaskId, &report);
        }
    }

    void service() {
        RegisterPseudoService registerRequest {};
        registerRequest.type = ServiceType::Memory;
        registerRequest.handler = handleMessage;

        send(IPC::RecipientType::ServiceRegistryMailbox, &registerRequest);
    }
}