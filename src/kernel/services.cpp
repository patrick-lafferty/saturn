/*
Copyright (c) 2017, Patrick Lafferty
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

    * Redistributions of source code must retain the above copyright
      notice, this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright
      notice, this list of conditions and the following disclaimer in the
      documentation and/or other materials provided with the distribution.
    * Neither the name of the copyright holder nor the names of its 
      contributors may be used to endorse or promote products derived from 
      this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR 
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/
#include "services.h"
#include <string.h>
#include <scheduler.h>
#include <memory/virtual_memory_manager.h>
#include <memory/physical_memory_manager.h>
#include <stdio.h>
#include "permissions.h"
#include <cpu/tss.h>

namespace Kernel {

    ServiceRegistry::ServiceRegistry() {
        auto count = static_cast<uint32_t>(ServiceType::ServiceTypeEnd) + 1;
        taskIds = new uint32_t[count];
        memset(taskIds, 0, count * sizeof(uint32_t));

        driverTaskIds = new uint32_t[count];
        memset(driverTaskIds, 0, count * sizeof(uint32_t));

        for (auto i = 0u; i < count; i++) {
            meta.push_back({});
            subscribers.push_back({});
        }

        pseudoMessageHandlers = new PseudoMessageHandler[count];
    }

    void ServiceRegistry::receiveMessage(IPC::Message* message) {
        switch(message->messageNamespace) {
            case IPC::MessageNamespace::ServiceRegistry: {
                switch(static_cast<MessageId>(message->messageId)) {
                    case MessageId::RegisterService: {
                        auto request = IPC::extractMessage<RegisterService>(
                            *static_cast<IPC::MaximumMessageBuffer*>(message));
                        registerService(request.senderTaskId, request.type);

                        break;
                    }
                    case MessageId::RegisterPseudoService: {
                        auto request = IPC::extractMessage<RegisterPseudoService>(
                            *static_cast<IPC::MaximumMessageBuffer*>(message));
                        registerPseudoService(request.type, request.handler);

                        break;
                    }
                    case MessageId::SubscribeServiceRegistered: {
                        auto request = IPC::extractMessage<SubscribeServiceRegistered>(
                            *static_cast<IPC::MaximumMessageBuffer*>(message));
                        
                        auto index = static_cast<uint32_t>(request.type);
                        
                        subscribers[index].push_back(request.senderTaskId);

                        if (meta[index].ready) {
                            notifySubscribers(index);
                        }

                        break;
                    }
                    case MessageId::RunProgram: {
                        auto run = IPC::extractMessage<RunProgram>(
                            *static_cast<IPC::MaximumMessageBuffer*>(message));
                        
                        char* path = nullptr;

                        if (run.path[0] != '\0') {
                            path = run.path;
                        }

                        //kprintf("[ServiceRegistry] runProgram\n");

                        auto task = currentScheduler->createUserTask(run.entryPoint, path);
                        currentScheduler->scheduleTask(task);

                        RunResult result;
                        result.recipientId = message->senderTaskId;
                        result.success = task != nullptr;
                        result.pid = task->id;
                        auto currentTask = currentScheduler->getTask(message->senderTaskId);
                        currentTask->mailbox->send(&result);

                        break;
                    }
                    case MessageId::NotifyServiceReady: {
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

                        break;
                    }
                    case MessageId::LinearFrameBufferFound: {
                        auto msg = IPC::extractMessage<LinearFrameBufferFound>(
                                *static_cast<IPC::MaximumMessageBuffer*>(message));

                        addresses.linearFrameBuffer = msg.address;

                        break;
                    }
                    case MessageId::RegisterDriver: {
                        auto request = IPC::extractMessage<RegisterDriver>(
                            *static_cast<IPC::MaximumMessageBuffer*>(message));
                        
                        registerDriver(request.senderTaskId, request.type);

                        break;
                    }
                    case MessageId::MapMemory: {
                        auto request = IPC::extractMessage<MapMemory>(
                            *static_cast<IPC::MaximumMessageBuffer*>(message));

                        auto currentTask = currentScheduler->getTask(message->senderTaskId);
                        auto vmm = currentTask->virtualMemoryManager;
                        auto cachedNextAddress = vmm->HACK_getNextAddress();

                        /*
                        TODO: just a hacked up implementation to get the elf loader test working
                        */
                        vmm->HACK_setNextAddress(request.address);
                        auto allocatedAddress = vmm->allocatePages(request.size, request.flags);
                        vmm->HACK_setNextAddress(cachedNextAddress);

                        MapMemoryResult result;
                        result.recipientId = request.senderTaskId;
                        result.start = reinterpret_cast<void*>(allocatedAddress);

                        currentTask->mailbox->send(&result);
                        
                        break;
                    }
                    case MessageId::ShareMemory: {
                        auto request = IPC::extractMessage<ShareMemory>(
                            *static_cast<IPC::MaximumMessageBuffer*>(message));

                        auto currentTask = currentScheduler->getTask(message->senderTaskId);
                        auto recipientTask = currentScheduler->getTask(request.sharedTaskId);

                        currentTask->virtualMemoryManager->sharePages(
                            request.ownerAddress,
                            recipientTask->virtualMemoryManager,
                            request.sharedAddress,
                            request.size / Memory::PageSize
                        );

                        ShareMemoryResult result;
                        result.recipientId = request.senderTaskId;
                        result.sharedTaskId = request.sharedTaskId;

                        currentTask->mailbox->send(&result);
                    }
                }
                break;
            }
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

    bool ServiceRegistry::registerDriver(uint32_t taskId, DriverType type) {
        if (type == DriverType::DriverTypeEnd) {
            kprintf("[ServiceRegistry] Tried to register DriverTypeEnd\n");
            return false;
        }

        auto index = static_cast<uint32_t>(type);

        if (driverTaskIds[index] != 0) {
            kprintf("[ServiceRegistry] Tried to register a driver[%d] that's taken\n", index);
            return false;
        }

        setupDriver(taskId, type);

        driverTaskIds[index] = taskId;
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

    bool ServiceRegistry::handleDriverIrq(uint32_t irq) {
        switch(irq) {
            case 53: {
                auto taskId = driverTaskIds[static_cast<uint32_t>(DriverType::ATA)];

                if (taskId != 0) {
                    DriverIrqReceived msg;
                    msg.recipientId = taskId;
                    currentScheduler->sendMessage(IPC::RecipientType::TaskId, &msg);

                    return true;
                }
            }
        }

        return false;
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
            case ServiceType::WindowManager: {
                auto task = currentScheduler->getTask(taskId);

                if (task == nullptr) {
                    kprintf("[ServiceRegistry] Tried to setupService a null task\n");
                    return;
                }

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

    void ServiceRegistry::setupDriver(uint32_t taskId, DriverType type) {
asm("cli");
        switch (type) {
            
            case DriverType::ATA: {
                
                auto task = currentScheduler->getTask(taskId);

                if (task == nullptr) {
                    kprintf("[ServiceRegistry] Tried to setupDriver a null task\n");
                    return;
                }

                //kprintf("[ServiceRegistry] tss %x grant: %x\n", task, task->tss);
                auto oldVMM = Memory::currentVMM;
                task->virtualMemoryManager->activate();
                grantIOPortRange(0x1f0, 0x1f7, task->tss->ioPermissionBitmap);
                grantIOPort8(0x3f6, task->tss->ioPermissionBitmap);

                oldVMM->activate();

                RegisterDriverResult result;
                task->mailbox->send(&result);

                break;
            }
            case DriverType::DriverTypeEnd: {
                kprintf("[ServiceRegistry] Tried to setupDriver a DriverTypeEnd\n");
                break;
            }
            default: {
                kprintf("[ServiceRegistry] Unsupported service type\n");   
            }
        }
asm("sti");
    }
}