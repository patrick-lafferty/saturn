#include "services.h"
#include <string.h>
#include <scheduler.h>
#include <memory/virtual_memory_manager.h>

namespace Kernel {

    uint32_t RegisterService::MessageId;
    uint32_t RegisterServiceDenied::MessageId;
    uint32_t VGAServiceMeta::MessageId;

    ServiceRegistry::ServiceRegistry() {
        auto count = static_cast<uint32_t>(ServiceType::ServiceTypeEnd) + 1;
        taskIds = new uint32_t[count];
        memset(taskIds, 0, count * sizeof(uint32_t));

        meta = new ServiceMeta*[count];

        IPC::registerMessage<RegisterService>();
        IPC::registerMessage<RegisterServiceDenied>();
        IPC::registerMessage<VGAServiceMeta>();
    }

    void ServiceRegistry::receiveMessage(IPC::Message* message) {
        if (message->messageId == RegisterService::MessageId) {
            auto request = IPC::extractMessage<RegisterService>(
                    *static_cast<IPC::MaximumMessageBuffer*>(message));
            registerService(request.senderTaskId, request.type);
        }
    }

    bool ServiceRegistry::registerService(uint32_t taskId, ServiceType type) {
        if (type == ServiceType::ServiceTypeEnd) {
            return false;
        }

        auto index = static_cast<uint32_t>(type);

        if (taskIds[index] != 0) {
            return false;
        }

        setupService(taskId, type);

        taskIds[index] = taskId;
        return true;
    }

    uint32_t ServiceRegistry::getServiceTaskId(ServiceType type) {
        if (type == ServiceType::ServiceTypeEnd) {
            return 0;
        }

        return taskIds[static_cast<uint32_t>(type)];
    }

    void ServiceRegistry::setupService(uint32_t taskId, ServiceType type) {
        //TODO: iopb/page map

        switch (type) {
            case ServiceType::VGA: {
                auto task = currentScheduler->getTask(taskId);

                if (task == nullptr) {
                    return;
                }

                auto vgaPage = task->virtualMemoryManager->allocatePages(1);
                auto pageFlags = 
                    static_cast<int>(Memory::PageTableFlags::Present)
                    | static_cast<int>(Memory::PageTableFlags::AllowWrite)
                    | static_cast<int>(Memory::PageTableFlags::AllowUserModeAccess);
                task->virtualMemoryManager->map(vgaPage, 0xB8000, pageFlags);

                auto vgaMeta = new VGAServiceMeta;
                vgaMeta->vgaAddress = vgaPage;
                meta[static_cast<uint32_t>(type)] = vgaMeta;
                vgaMeta->length = sizeof(VGAServiceMeta);
                vgaMeta->messageId = VGAServiceMeta::MessageId;
                task->mailbox->send(vgaMeta);

                break;
            }
            case ServiceType::ServiceTypeEnd: {
                break;
            }
        }

    }
}