#include "services.h"
#include <string.h>
#include <scheduler.h>
#include <memory/virtual_memory_manager.h>
#include <stdio.h>
#include "permissions.h"
#include <cpu/tss.h>

namespace Kernel {

    uint32_t RegisterService::MessageId;
    uint32_t RegisterPseudoService::MessageId;
    uint32_t RegisterServiceDenied::MessageId;
    uint32_t NotifyServiceReady::MessageId;
    uint32_t VGAServiceMeta::MessageId;
    uint32_t GenericServiceMeta::MessageId;
    uint32_t SubscribeServiceRegistered::MessageId;
    uint32_t NotifyServiceRegistered::MessageId;
    uint32_t RunProgram::MessageId;
    uint32_t RunResult::MessageId;
    uint32_t LinearFrameBufferFound::MessageId;

    ServiceRegistry::ServiceRegistry() {
        auto count = static_cast<uint32_t>(ServiceType::ServiceTypeEnd) + 1;
        taskIds = new uint32_t[count];
        memset(taskIds, 0, count * sizeof(uint32_t));

        for (auto i = 0u; i < count; i++) {
            meta.push_back({});
            subscribers.push_back({});
        }

        pseudoMessageHandlers = new PseudoMessageHandler[count];

        IPC::registerMessage<RegisterService>();
        IPC::registerMessage<RegisterPseudoService>();
        IPC::registerMessage<RegisterServiceDenied>();
        IPC::registerMessage<NotifyServiceReady>();
        IPC::registerMessage<VGAServiceMeta>();
        IPC::registerMessage<GenericServiceMeta>();
        IPC::registerMessage<SubscribeServiceRegistered>();
        IPC::registerMessage<NotifyServiceRegistered>();
        IPC::registerMessage<RunProgram>();
        IPC::registerMessage<RunResult>();
        IPC::registerMessage<LinearFrameBufferFound>();
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
        else if (message->messageId == SubscribeServiceRegistered::MessageId) {
            auto request = IPC::extractMessage<SubscribeServiceRegistered>(
                *static_cast<IPC::MaximumMessageBuffer*>(message));
            
            auto index = static_cast<uint32_t>(request.type);
            
            subscribers[index].push_back(request.senderTaskId);

            if (meta[index].ready) {
                notifySubscribers(index);
            }
        }
        else if (message->messageId == RunProgram::MessageId) {
            auto run = IPC::extractMessage<RunProgram>(
                *static_cast<IPC::MaximumMessageBuffer*>(message));
            
            auto task = currentScheduler->createUserTask(run.entryPoint);
            currentScheduler->scheduleTask(task);

            RunResult result;
            result.recipientId = message->senderTaskId;
            result.success = task != nullptr;
            result.pid = task->id;
            auto currentTask = currentScheduler->getTask(message->senderTaskId);
            currentTask->mailbox->send(&result);
        }
        else if (message->messageId == NotifyServiceReady::MessageId) {
            auto lastService = static_cast<uint32_t>(ServiceType::ServiceTypeEnd);
            auto index = lastService;

            for (auto i = 0u; i < lastService; i++) {
                if (taskIds[i] == message->senderTaskId) {
                    index = i;
                    break;
                }
            }

            if (index != lastService) {
                meta[index].ready = true;
                notifySubscribers(index);
            }
        }
        else if (message->messageId == LinearFrameBufferFound::MessageId) {
            auto msg = IPC::extractMessage<LinearFrameBufferFound>(
                    *static_cast<IPC::MaximumMessageBuffer*>(message));

            addresses.linearFrameBuffer = msg.address;
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
            kprintf("[ServiceRegistry] Tried to register ServiceTypeEnd\n");
            return false;
        }

        auto index = static_cast<uint32_t>(type);

        if (taskIds[index] != 0) {
            kprintf("[ServiceRegistry] Tried to register a service[%d] that's taken\n", index);
            return false;
        }

        setupService(taskId, type);

        taskIds[index] = taskId;
        return true;
    }

    bool ServiceRegistry::registerPseudoService(ServiceType type, PseudoMessageHandler handler) {
        if (type == ServiceType::ServiceTypeEnd) {
            kprintf("[ServiceRegistry] Tried to register PseudoServiceTypeEnd\n");
            return false;
        }

        auto index = static_cast<uint32_t>(type);

        if (pseudoMessageHandlers[index] != nullptr) {
            kprintf("[ServiceRegistry] Tried to register a pseudo service that's taken\n");
            return false;
        }

        pseudoMessageHandlers[index] = handler;
        return true;
    }

    uint32_t ServiceRegistry::getServiceTaskId(ServiceType type) {
        if (type == ServiceType::ServiceTypeEnd) {
            kprintf("[ServiceRegistry] Tried to get ServiceTypeEnd taskId\n");
            return 0;
        }

        return taskIds[static_cast<uint32_t>(type)];
    }

    bool ServiceRegistry::isPseudoService(ServiceType type) {
        auto index = static_cast<uint32_t>(type);

        return pseudoMessageHandlers[index] != nullptr;
    }

    void ServiceRegistry::notifySubscribers(uint32_t index) {
        for (auto taskId : subscribers[index]) {
            NotifyServiceRegistered notify;
            notify.type = static_cast<ServiceType>(index);
            notify.recipientId = taskId;

            currentScheduler->sendMessage(IPC::RecipientType::TaskId, &notify);
        }

        subscribers[index].clear();
    }

    void ServiceRegistry::setupService(uint32_t taskId, ServiceType type) {
        //TODO: iopb/page map

        switch (type) {
            case ServiceType::VGA: {
                auto task = currentScheduler->getTask(taskId);

                if (task == nullptr) {
                    kprintf("[ServiceRegistry] Tried to setupService a null task\n");
                    return;
                }

                auto vgaPage = task->virtualMemoryManager->allocatePages(1);
                auto pageFlags = 
                    static_cast<int>(Memory::PageTableFlags::Present)
                    | static_cast<int>(Memory::PageTableFlags::AllowWrite)
                    | static_cast<int>(Memory::PageTableFlags::AllowUserModeAccess);
                task->virtualMemoryManager->map(vgaPage, 0xB8000, pageFlags);

                VGAServiceMeta vgaMeta;
                vgaMeta.vgaAddress = vgaPage;
                task->mailbox->send(&vgaMeta);

                break;
            }
            case ServiceType::Terminal:
            case ServiceType::PS2:
            case ServiceType::Keyboard:
            case ServiceType::Mouse: 
            case ServiceType::VFS: {

                auto task = currentScheduler->getTask(taskId);

                if (task == nullptr) {
                    kprintf("[ServiceRegistry] Tried to setupService a null task\n");
                    return;
                }

                GenericServiceMeta genericMeta;
                task->mailbox->send(&genericMeta);

                break;
            }
            case ServiceType::BGA: {
                
                auto task = currentScheduler->getTask(taskId);

                if (task == nullptr) {
                    kprintf("[ServiceRegistry] Tried to setupService a null task\n");
                    return;
                }

                grantIOPort16(0x1ce, task->tss->ioPermissionBitmap);
                grantIOPort16(0x1cf, task->tss->ioPermissionBitmap);

                auto linearFrameBuffer = task->virtualMemoryManager->allocatePages(470);
                auto pageFlags = 
                    static_cast<int>(Memory::PageTableFlags::Present)
                    | static_cast<int>(Memory::PageTableFlags::AllowWrite)
                    | static_cast<int>(Memory::PageTableFlags::AllowUserModeAccess);

                for (int i = 0; i < 470; i++) {
                    task->virtualMemoryManager->map(linearFrameBuffer + 0x1000 * i, addresses.linearFrameBuffer + 0x1000 * i, pageFlags);
                }

                VGAServiceMeta meta;
                meta.vgaAddress = linearFrameBuffer;
                task->mailbox->send(&meta);

                break;
            }
            case ServiceType::ServiceTypeEnd: {
                kprintf("[ServiceRegistry] Tried to setupService a ServiceTypeEnd\n");
                break;
            }
            default: {
                kprintf("[ServiceRegistry] Unsupported service type\n");   
            }
        }
    }
}