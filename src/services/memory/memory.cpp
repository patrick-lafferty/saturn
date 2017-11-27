#include "memory.h"
#include <system_calls.h>
#include <services.h>
#include <memory/physical_memory_manager.h>

using namespace Kernel;

namespace Memory {

    uint32_t GetPhysicalReport::MessageId;
    uint32_t PhysicalReport::MessageId;

    void registerMessages() {
        IPC::registerMessage<GetPhysicalReport>();
        IPC::registerMessage<PhysicalReport>();
    }

    void handleMessage(IPC::Message* message) {

        if (message->messageId == GetPhysicalReport::MessageId) {
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

        registerMessages();
    }
}