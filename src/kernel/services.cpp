#include "services.h"
#include <string.h>
#include <scheduler.h>
#include <memory/virtual_memory_manager.h>
#include <stdio.h>

namespace Kernel {

    uint32_t RegisterService::MessageId;
    uint32_t RegisterPseudoService::MessageId;
    uint32_t RegisterServiceDenied::MessageId;
    uint32_t VGAServiceMeta::MessageId;
    uint32_t GenericServiceMeta::MessageId;

    ServiceRegistry::ServiceRegistry() {
        auto count = static_cast<uint32_t>(ServiceType::ServiceTypeEnd) + 1;
        taskIds = new uint32_t[count];
        memset(taskIds, 0, count * sizeof(uint32_t));

        meta = new ServiceMeta*[count];

        pseudoMessageHandlers = new PseudoMessageHandler[count];

        IPC::registerMessage<RegisterService>();
        IPC::registerMessage<RegisterPseudoService>();
        IPC::registerMessage<RegisterServiceDenied>();
        IPC::registerMessage<VGAServiceMeta>();
        IPC::registerMessage<GenericServiceMeta>();
    }

    void ServiceRegistry::receiveMessage(IPC::Message* message) {
        if (message->messageId == RegisterService::MessageId) {
            auto request = IPC::extractMessage<RegisterService>(
                *static_cast<IPC::MaximumMessageBuffer*>(message));
            registerService(request.senderTaskId, request.type);
        }
        else if (message->messageId == RegisterPseudoService::MessageId) {
            auto request = IPC::extractMessage<RegisterPseudoService>(
                *static_cast<IPC::MaximumMessageBuffer*>(message));
            registerPseudoService(request.type, request.handler);
        }
    }

    void ServiceRegistry::receivePseudoMessage(ServiceType type, IPC::Message* message) {
        auto index = static_cast<uint32_t>(type);

        if (pseudoMessageHandlers[index] != nullptr) {
            pseudoMessageHandlers[index](message);
        }
    }

    bool ServiceRegistry::registerService(uint32_t taskId, ServiceType type) {
        if (type == ServiceType::ServiceTypeEnd) {
            printf("[ServiceRegistry] Tried to register ServiceTypeEnd\n");
            return false;
        }

        auto index = static_cast<uint32_t>(type);

        if (taskIds[index] != 0) {
            printf("[ServiceRegistry] Tried to register a service that's taken\n");
            return false;
        }

        setupService(taskId, type);

        taskIds[index] = taskId;
        return true;
    }

    bool ServiceRegistry::registerPseudoService(ServiceType type, PseudoMessageHandler handler) {
        if (type == ServiceType::ServiceTypeEnd) {
            printf("[ServiceRegistry] Tried to register PseudoServiceTypeEnd\n");
            return false;
        }

        auto index = static_cast<uint32_t>(type);

        if (pseudoMessageHandlers[index] != nullptr) {
            printf("[ServiceRegistry] Tried to register a pseudo service that's taken\n");
            return false;
        }

        pseudoMessageHandlers[index] = handler;
        return true;
    }

    uint32_t ServiceRegistry::getServiceTaskId(ServiceType type) {
        if (type == ServiceType::ServiceTypeEnd) {
            printf("[ServiceRegistry] Tried to get ServiceTypeEnd taskId\n");
            return 0;
        }

        return taskIds[static_cast<uint32_t>(type)];
    }

    bool ServiceRegistry::isPseudoService(ServiceType type) {
        auto index = static_cast<uint32_t>(type);

        return pseudoMessageHandlers[index] != nullptr;
    }

    void ServiceRegistry::setupService(uint32_t taskId, ServiceType type) {
        //TODO: iopb/page map

        switch (type) {
            case ServiceType::VGA: {
                auto task = currentScheduler->getTask(taskId);

                if (task == nullptr) {
                    printf("[ServiceRegistry] Tried to setupService a null task\n");
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
                task->mailbox->send(vgaMeta);

                break;
            }
            case ServiceType::Terminal:
            case ServiceType::PS2:
            case ServiceType::Keyboard:
            case ServiceType::Mouse: {

                auto task = currentScheduler->getTask(taskId);

                if (task == nullptr) {
                    printf("[ServiceRegistry] Tried to setupService a null task\n");
                    return;
                }

                auto genericMeta = new GenericServiceMeta;
                meta[static_cast<uint32_t>(type)] = genericMeta;
                task->mailbox->send(genericMeta);

                break;
            }
            case ServiceType::ServiceTypeEnd: {
                printf("[ServiceRegistry] Tried to setupService a ServiceTypeEnd\n");
                break;
            }
        }

    }
}