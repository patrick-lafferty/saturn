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
#include <memory/virtual_memory_manager.h>
#include <memory/physical_memory_manager.h>
#include <stdio.h>
#include "permissions.h"
#include <cpu/tss.h>
#include <cpu/cpu.h>
#include <task.h>
#include <system_calls.h>
#include <memory/guard.h>

namespace Kernel {

    ServiceRegistry::ServiceRegistry() 
        : subscriberAllocator {Memory::getCurrentVMM()} {

        kernelVMM = Memory::getCurrentVMM();

        auto count = static_cast<uint32_t>(ServiceType::ServiceTypeEnd) + 1;
        BlockAllocator<ServiceHandle> allocator {kernelVMM};
        knownServices = allocator.allocateMultiple(count);

        BlockAllocator<uint32_t> intAllocator {kernelVMM};
        driverTaskIds = intAllocator.allocateMultiple(count);

        for (int i = 0; i < count; i++) {

            knownServices[i].subscribers = subscriberAllocator.allocateMultiple(knownServices[i].MaxSubscribers);
        }
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
                    case MessageId::SubscribeServiceRegistered: {
                        auto request = IPC::extractMessage<SubscribeServiceRegistered>(
                            *static_cast<IPC::MaximumMessageBuffer*>(message));
                        
                        auto index = static_cast<uint32_t>(request.type);
                        subscribe(index, request.senderTaskId); 

                        break;
                    }
                    case MessageId::RunProgram: {

                        break;
                    }
                    case MessageId::NotifyServiceReady: {
                        
                        handleNotifyServiceReady(message->senderTaskId);
                        break;
                    }
                    case MessageId::LinearFrameBufferFound: {
                        auto msg = IPC::extractMessage<LinearFrameBufferFound>(
                                *static_cast<IPC::MaximumMessageBuffer*>(message));

                        handleLinearFramebufferFound(msg.address);

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
                        
                        handleMapMemory(request);
                        
                        break;
                    }
                    case MessageId::ShareMemoryRequest: {
                        auto request = IPC::extractMessage<ShareMemoryRequest>(
                            *static_cast<IPC::MaximumMessageBuffer*>(message));

                        handleShareMemoryRequest(request);

                        break;
                    }
                    default: {
                        printf("[ServiceRegistry] Unhandled message id\n");
                    }
                }
                break;
            }
            default: {
                printf("[ServiceRegistry] Unhandled message namespace\n");
            }
        }
    }

    bool ServiceRegistry::registerService(uint32_t taskId, ServiceType type) {
        if (type == ServiceType::ServiceTypeEnd) {
            kprintf("[ServiceRegistry] Tried to register ServiceTypeEnd\n");
            return false;
        }

        auto index = static_cast<uint32_t>(type);

        if (knownServices[index].taskId != 0) {
            kprintf("[ServiceRegistry] Tried to register a service[%d] that's taken\n", index);
            return false;
        }

        setupService(taskId, type);

        knownServices[index].taskId = taskId;
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

    void ServiceRegistry::subscribe(uint32_t index, uint32_t senderTaskId) {

        MemoryGuard guard {kernelVMM};

        auto& service = knownServices[index];

        if (service.subscribers == nullptr) {
            service.subscribers = subscriberAllocator.allocateMultiple(service.MaxSubscribers);
        }

        //todo: check if exceeded max subscriber count
        service.subscribers[service.subscriberCount] = senderTaskId;
        service.subscriberCount++;

        if (service.isReady) {
            notifySubscribers(index);
        }
    }

    void ServiceRegistry::handleNotifyServiceReady(uint32_t senderTaskId) {

        MemoryGuard guard {kernelVMM};

        auto lastService = static_cast<uint32_t>(ServiceType::ServiceTypeEnd);

        for (auto i = 0u; i < lastService; i++) {
            if (knownServices[i].taskId == senderTaskId) {
                knownServices[i].isReady = true;
                notifySubscribers(i);
                break;
            }
        }
    }

    void ServiceRegistry::handleLinearFramebufferFound(uint32_t address) {

        MemoryGuard guard {kernelVMM};

        addresses.linearFrameBuffer = address;
    }

    void handleMapMemory(MapMemory request) {

        /*
        handleMapMemory doesn't need a MemoryGuard because it
        doesn't touch any member variables, and indeed it expects
        currentTask's VMM to be activated, not the kernelVMM
        */

        auto currentTask = CPU::getTask(request.senderTaskId);
        auto vmm = currentTask->virtualMemoryManager;
        auto cachedNextAddress = vmm->HACK_getNextAddress();

        /*
        TODO: just a hacked up implementation to get the elf loader test working
        */
        vmm->HACK_setNextAddress(request.address);
        auto allocatedAddress = vmm->allocatePages(request.size, request.flags);
        vmm->HACK_setNextAddress(cachedNextAddress);

        MapMemoryResult result;
        result.senderTaskId = request.senderTaskId;
        result.recipientId = request.senderTaskId;
        result.start = reinterpret_cast<void*>(allocatedAddress);

        currentTask->mailbox->send(&result);
    }

    void handleShareMemoryRequest(ShareMemoryRequest request) {

        /*
        handleShareMemory doesn't need a MemoryGuard because it
        doesn't touch any member variables, and indeed it expects
        currentTask's VMM to be activated, not the kernelVMM
        */

        auto currentTask = CPU::getTask(request.senderTaskId);

        ShareMemoryInvitation invitation;
        auto startTableIndex = request.ownerAddress / Memory::PageSize;
        auto endTableIndex = (request.ownerAddress + request.size) / Memory::PageSize;
        auto tablesRequired = 1 + (endTableIndex - startTableIndex);

        invitation.size = tablesRequired * Memory::PageSize;

        if (request.recipientIsTaskId) {

            invitation.recipientId = request.sharedTaskId;
            send(IPC::RecipientType::TaskId, &invitation);
        }
        else {
            invitation.serviceType = request.sharedServiceType;
            send(IPC::RecipientType::ServiceName, &invitation);
        }

        IPC::MaximumMessageBuffer buffer;
        filteredReceive(&buffer, IPC::MessageNamespace::ServiceRegistry, static_cast<uint32_t>(MessageId::ShareMemoryResponse));

        auto response = IPC::extractMessage<ShareMemoryResponse>(buffer);

        ShareMemoryResult result;
        result.recipientId = request.senderTaskId;
        result.sharedTaskId = request.sharedTaskId;
        result.succeeded = response.accepted;
        result.senderTaskId = request.senderTaskId;
        result.pageOffset = request.ownerAddress % Memory::PageSize;

        currentTask->mailbox->send(&result);

        if (response.accepted) {
            Task* recipientTask;

            if (request.recipientIsTaskId) {
                recipientTask = CPU::getTask(request.sharedTaskId);
            }
            else {
                auto serviceId = ServiceRegistryInstance->getServiceTaskId(request.sharedServiceType);
                recipientTask = CPU::getTask(serviceId);
            }

            currentTask->virtualMemoryManager->sharePages(
                request.ownerAddress,
                recipientTask->virtualMemoryManager,
                response.sharedAddress,
                tablesRequired
            );

            recipientTask->mailbox->send(&result);
        }
    }

    uint32_t ServiceRegistry::getServiceTaskId(ServiceType type) {

        MemoryGuard guard {kernelVMM};

        if (type == ServiceType::ServiceTypeEnd) {
            kprintf("[ServiceRegistry] Tried to get ServiceTypeEnd taskId\n");
            return 0;
        }

        return knownServices[static_cast<uint32_t>(type)].taskId;
    }

    bool ServiceRegistry::handleDriverIrq(uint32_t irq) {

        MemoryGuard guard {kernelVMM};

        switch(irq) {
            case 53: {
                auto taskId = driverTaskIds[static_cast<uint32_t>(DriverType::ATA)];

                if (taskId != 0) {
                    DriverIrqReceived msg;
                    msg.recipientId = taskId;
                    CPU::sendMessage(IPC::RecipientType::TaskId, &msg);

                    return true;
                }
            }
        }

        return false;
    }

    void ServiceRegistry::notifySubscribers(uint32_t index) {

        auto& service = knownServices[index];

        for (int i = 0; i < service.subscriberCount; i++) {
            if (service.subscribers == nullptr) {
                asm("cli");
                asm("hlt");
            }
            NotifyServiceRegistered notify;
            notify.type = static_cast<ServiceType>(index);
            notify.recipientId = service.subscribers[i];

            CPU::sendMessage(IPC::RecipientType::TaskId, &notify);
        }

        subscriberAllocator.freeMultiple(service.subscribers, service.MaxSubscribers);
        service.subscribers = nullptr;
        service.subscriberCount = 0;
    }

    void ServiceRegistry::setupService(uint32_t taskId, ServiceType type) {

        switch (type) {
            case ServiceType::Scheduler: {
                auto task = CPU::getTask(taskId);

                if (task == nullptr) {
                    kprintf("[ServiceRegistry] Tried to setupService a null task\n");
                    asm("hlt");
                    return;
                }

                GenericServiceMeta genericMeta;
                genericMeta.senderTaskId = taskId;
                task->mailbox->send(&genericMeta); 

                break;
            }
            case ServiceType::VGA: {
                auto task = CPU::getTask(taskId);

                if (task == nullptr) {
                    kprintf("[ServiceRegistry] Tried to setupService a null task\n");
                    asm("hlt");
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
                vgaMeta.senderTaskId = taskId;
                task->mailbox->send(&vgaMeta);

                break;
            }
            case ServiceType::PS2: {  
                auto task = CPU::getTask(taskId);
                CPU::changePriority(task, Priority::IRQ);

                if (task == nullptr) {
                    kprintf("[ServiceRegistry] Tried to setupService a null task\n");
                    asm("hlt");
                    return;
                }

                grantIOPort8(0x60, task->tss->ioPermissionBitmap);
                grantIOPort8(0x64, task->tss->ioPermissionBitmap);

                GenericServiceMeta genericMeta;
                genericMeta.senderTaskId = taskId;
                task->mailbox->send(&genericMeta);
                
                break;
            }
            case ServiceType::Terminal:
            case ServiceType::Keyboard:
            case ServiceType::Mouse: 
            case ServiceType::VFS: {

                auto task = CPU::getTask(taskId);

                if (task == nullptr) {
                    kprintf("[ServiceRegistry] Tried to setupService a null task\n");
                    asm("hlt");
                    return;
                }

                CPU::changePriority(task, type == ServiceType::VFS ? Priority::IO : Priority::Input);

                GenericServiceMeta genericMeta;
                genericMeta.senderTaskId = taskId;
                task->mailbox->send(&genericMeta);

                break;
            }
            case ServiceType::BGA: {
                
                auto task = CPU::getTask(taskId);

                if (task == nullptr) {
                    kprintf("[ServiceRegistry] Tried to setupService a null task\n");
                    asm("hlt");
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
                meta.senderTaskId = taskId;
                task->mailbox->send(&meta);

                break;
            }
            case ServiceType::WindowManager: {
                auto task = CPU::getTask(taskId);

                if (task == nullptr) {
                    kprintf("[ServiceRegistry] Tried to setupService a null task\n");
                    asm("hlt");
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
                meta.senderTaskId = taskId;
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
                
                auto task = CPU::getTask(taskId);
                CPU::changePriority(task, Priority::IRQ);

                if (task == nullptr) {
                    kprintf("[ServiceRegistry] Tried to setupDriver a null task\n");
                    return;
                }

                auto oldVMM = Memory::getCurrentVMM();
                task->virtualMemoryManager->activate();
                grantIOPortRange(0x1f0, 0x1f7, task->tss->ioPermissionBitmap);
                grantIOPort8(0x3f6, task->tss->ioPermissionBitmap);

                oldVMM->activate();

                RegisterDriverResult result;
                result.senderTaskId = taskId;
                task->mailbox->send(&result);

                break;
            }
            case DriverType::Serial: {
                
                auto task = CPU::getTask(taskId);

                if (task == nullptr) {
                    kprintf("[ServiceRegistry] Tried to setupDriver a null task\n");
                    return;
                }

                auto oldVMM = Memory::getCurrentVMM();
                task->virtualMemoryManager->activate();
                grantIOPortRange(0x3f8, 0x400, task->tss->ioPermissionBitmap);
                oldVMM->activate();

                RegisterDriverResult result;
                result.senderTaskId = taskId;
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
