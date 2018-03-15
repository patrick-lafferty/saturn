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
#pragma once

#include <stdint.h>
#include <ipc.h>
#include <vector>

namespace Memory {
    class VirtualMemoryManager;
}

namespace LibC_Implementation {
    class Heap;
}

namespace Kernel {

    enum class ServiceType {
        VGA,
        Terminal,
        PS2,
        Keyboard,
        Mouse,
        Memory,
        VFS,
        BGA,
        WindowManager,
        ServiceTypeEnd
    };

    enum class MessageId {
        RegisterService,
        RegisterDriver,
        RegisterDriverResult,
        DriverIrqReceived,
        RegisterPseudoService,
        RegisterServiceDenied,
        NotifyServiceReady,
        VGAServiceMeta,
        GenericServiceMeta,
        SubscribeServiceRegistered,
        NotifyServiceRegistered,
        RunProgram,
        RunResult,
        LinearFrameBufferFound,
        MapMemory,
        MapMemoryResult,
        ShareMemoryRequest,
        ShareMemoryInvitation,
        ShareMemoryResponse,
        ShareMemoryResult
    };

    struct RegisterService : IPC::Message {
        RegisterService() {
            messageId = static_cast<uint32_t>(MessageId::RegisterService);
            length = sizeof(RegisterService);
            messageNamespace = IPC::MessageNamespace::ServiceRegistry;
        }

        ServiceType type;
    };

    enum class DriverType {
        ATA,
        DriverTypeEnd
    };

    struct RegisterDriver : IPC::Message {
        RegisterDriver() {
            messageId = static_cast<uint32_t>(MessageId::RegisterDriver);
            length = sizeof(RegisterDriver);
            messageNamespace = IPC::MessageNamespace::ServiceRegistry;
        }

        DriverType type;
    };

    struct RegisterDriverResult : IPC::Message {
        RegisterDriverResult() {
            messageId = static_cast<uint32_t>(MessageId::RegisterDriverResult);
            length = sizeof(RegisterDriverResult);
            messageNamespace = IPC::MessageNamespace::ServiceRegistry;
        }

        bool success;
    };

    struct DriverIrqReceived : IPC::Message {
        DriverIrqReceived() {
            messageId = static_cast<uint32_t>(MessageId::DriverIrqReceived);
            length = sizeof(DriverIrqReceived);
            messageNamespace = IPC::MessageNamespace::ServiceRegistry;
        }

    };

    typedef void (*PseudoMessageHandler)(IPC::Message*);

    struct RegisterPseudoService : IPC::Message {
        RegisterPseudoService() {
            messageId = static_cast<uint32_t>(MessageId::RegisterPseudoService);
            length = sizeof(RegisterPseudoService);
            messageNamespace = IPC::MessageNamespace::ServiceRegistry;
        }

        ServiceType type;
        PseudoMessageHandler handler;
    };

    struct RegisterServiceDenied : IPC::Message {
        RegisterServiceDenied() {
            messageId = static_cast<uint32_t>(MessageId::RegisterServiceDenied);
            length = sizeof(RegisterServiceDenied);
            messageNamespace = IPC::MessageNamespace::ServiceRegistry;
        }

        bool success {false};
    };

    struct NotifyServiceReady : IPC::Message {
        NotifyServiceReady() {
            messageId = static_cast<uint32_t>(MessageId::NotifyServiceReady);
            length = sizeof(NotifyServiceReady);
            messageNamespace = IPC::MessageNamespace::ServiceRegistry;
        }

    };

    struct ServiceMeta : IPC::Message {
        ServiceMeta() {
            ready = false;
        }

        bool ready;
    };

    struct VGAServiceMeta : ServiceMeta {
        VGAServiceMeta() { 
            messageId = static_cast<uint32_t>(MessageId::VGAServiceMeta); 
            length = sizeof(VGAServiceMeta);
            messageNamespace = IPC::MessageNamespace::ServiceRegistry;
        }

        uint32_t vgaAddress {0};
    };

    struct GenericServiceMeta : ServiceMeta {
        GenericServiceMeta() {
            messageId = static_cast<uint32_t>(MessageId::GenericServiceMeta);
            length = sizeof(GenericServiceMeta);
            messageNamespace = IPC::MessageNamespace::ServiceRegistry;
        }

    };

    struct SubscribeServiceRegistered : IPC::Message {
        SubscribeServiceRegistered() {
            messageId = static_cast<uint32_t>(MessageId::SubscribeServiceRegistered);
            length = sizeof(SubscribeServiceRegistered);
            messageNamespace = IPC::MessageNamespace::ServiceRegistry;
        }

        ServiceType type;
    };

    struct NotifyServiceRegistered : IPC::Message {
        NotifyServiceRegistered() {
            messageId = static_cast<uint32_t>(MessageId::NotifyServiceRegistered);
            length = sizeof(NotifyServiceRegistered);
            messageNamespace = IPC::MessageNamespace::ServiceRegistry;
        }

        ServiceType type;
    };

    struct RunProgram : IPC::Message {
        RunProgram() {
            messageId = static_cast<uint32_t>(MessageId::RunProgram);
            length = sizeof(RunProgram);
            messageNamespace = IPC::MessageNamespace::Scheduler;
            memset(path, '\0', sizeof(path));
        }

        uintptr_t entryPoint;
        char path[256];
    };

    struct RunResult : IPC::Message {
        RunResult() {
            messageId = static_cast<uint32_t>(MessageId::RunResult);
            length = sizeof(RunResult);
            messageNamespace = IPC::MessageNamespace::ServiceRegistry;
        }

        bool success;
        uint32_t pid;
    };

    struct LinearFrameBufferFound : IPC::Message {
        LinearFrameBufferFound() {
            messageId = static_cast<uint32_t>(MessageId::LinearFrameBufferFound);
            length = sizeof(LinearFrameBufferFound);
            messageNamespace = IPC::MessageNamespace::ServiceRegistry;
        }

        uint32_t address;
    };

    struct MapMemory : IPC::Message {
        MapMemory() {
            messageId = static_cast<uint32_t>(MessageId::MapMemory);
            length = sizeof(MapMemory);
            messageNamespace = IPC::MessageNamespace::ServiceRegistry;
        }

        uint32_t address;
        uint32_t size;
        uint32_t flags;
    };

    struct MapMemoryResult : IPC::Message {
        MapMemoryResult() {
            messageId = static_cast<uint32_t>(MessageId::MapMemoryResult);
            length = sizeof(MapMemoryResult);
            messageNamespace = IPC::MessageNamespace::ServiceRegistry;
        }

        void* start;
    };

    /*
    NOTE: For the ShareXY messages,
    A refers to the task that wants to share memory
    B refers to the task that A wants to share with
    */
    struct ShareMemoryRequest : IPC::Message {
        ShareMemoryRequest() {
            messageId = static_cast<uint32_t>(MessageId::ShareMemoryRequest);
            length = sizeof(ShareMemoryRequest);
            messageNamespace = IPC::MessageNamespace::ServiceRegistry;
        }

        uintptr_t ownerAddress;

        union {
            uint32_t sharedTaskId;
            Kernel::ServiceType sharedServiceType;
        };

        bool recipientIsTaskId;
        uint32_t size;
    };

    /*
    Sent by ServiceRegistry on behalf of the task (A) that sent
    the initial ShareMemoryRequest, to the task (B) that A wants
    to share memory with.

    NOTE: Since memory sharing works per-page, size is always
    a multiple of Memory::PageSize. B must allocate page-aligned space
    to ensure it allocates completely unused pages. Don't just allocate
    <size> bytes, pretend you are allocating <size / PageSize> pages.

    Since the starting address of the space A wants to share might
    not be page aligned, eg its just a random buffer, ShareMemoryResult
    will give B the offset to the start of the buffer.
    */
    struct ShareMemoryInvitation : IPC::Message {
        ShareMemoryInvitation() {
            messageId = static_cast<uint32_t>(MessageId::ShareMemoryInvitation);
            length = sizeof(ShareMemoryInvitation);
            messageNamespace = IPC::MessageNamespace::ServiceRegistry;
        }

        /*
        Note: Always an integer multiple of Memory::PageSize.
        Allocate a page-aligned buffer with this size.
        */
        uint32_t size;
    };

    /*
    Upon receiving a ShareMemoryInvitation, B must send A 
    a response, with the address of the buffer B allocated
    when handling the invitation if accepted, or nothing
    if not accepted.
    */
    struct ShareMemoryResponse : IPC::Message {
        ShareMemoryResponse() {
            messageId = static_cast<uint32_t>(MessageId::ShareMemoryResponse);
            length = sizeof(ShareMemoryResponse);
            messageNamespace = IPC::MessageNamespace::ServiceRegistry;
        }

        uintptr_t sharedAddress;
        bool accepted;
    };

    /*
    Both A and B receive ShareMemoryResult once B sends
    a ShareMemoryResponse.
    */
    struct ShareMemoryResult : IPC::Message {
        ShareMemoryResult() {
            messageId = static_cast<uint32_t>(MessageId::ShareMemoryResult);
            length = sizeof(ShareMemoryResult);
            messageNamespace = IPC::MessageNamespace::ServiceRegistry;
        }

        uint32_t sharedTaskId;
        bool succeeded;

        /*
        Since B needs to allocate a page-aligned buffer, and A's
        starting address might not be page aligned, this stores
        the offset from the first page to the starting byte of
        A's shared address.
        */
        uint32_t pageOffset;
    };

    struct KnownHardwareAddresses {
        uint32_t linearFrameBuffer;    
    };

    inline uint32_t ServiceRegistryMailbox {0};

    /*
    A service is a usermode task that controls some frequently used
    operation, typically requiring special priviledges. There can
    only be one of each type. When the service task starts,
    it registers itself with the service which grants the appropriate
    permissions, and then records its taskId so that other tasks
    can refer to it by name and not taskId.
    */
    inline class ServiceRegistry* ServiceRegistryInstance;
    class ServiceRegistry {
    public:
        
        ServiceRegistry();
        
        /*
        All public functions must create a MemoryGuard at the beginning
        of the function.

        ServiceRegistry allocates memory on the kernel heap, but doesn't run
        in its own service, so the current heap when invoking a registry
        function is most likely a user task's heap, so the registry's
        member variables would be garbage.
        */
        void receiveMessage(IPC::Message* message);
        void receivePseudoMessage(ServiceType type, IPC::Message* message);
        uint32_t getServiceTaskId(ServiceType type);
        bool isPseudoService(ServiceType type);
        bool handleDriverIrq(uint32_t irq);

    private:
        
        bool registerService(uint32_t taskId, ServiceType type);
        bool registerDriver(uint32_t taskId, DriverType type);
        bool registerPseudoService(ServiceType type, PseudoMessageHandler handler);
        void subscribe(uint32_t index, uint32_t senderTaskId);
        void handleNotifyServiceReady(uint32_t senderTaskId);
        void handleLinearFramebufferFound(uint32_t address);
        void notifySubscribers(uint32_t index);
        void setupService(uint32_t taskId, ServiceType type);
        void setupDriver(uint32_t taskId, DriverType type);

        uint32_t* taskIds;
        std::vector<ServiceMeta> meta;
        PseudoMessageHandler* pseudoMessageHandlers;
        std::vector<std::vector<uint32_t>> subscribers;
        uint32_t* driverTaskIds;

        KnownHardwareAddresses addresses;

        Memory::VirtualMemoryManager* kernelVMM;
        LibC_Implementation::Heap* kernelHeap;

    };

    void handleMapMemory(MapMemory request);
    void handleShareMemoryRequest(ShareMemoryRequest request);

    class MemoryGuard {
    public:

        MemoryGuard(Memory::VirtualMemoryManager* kernelVMM, LibC_Implementation::Heap* kernelHeap);
        ~MemoryGuard();

    private:
        
        Memory::VirtualMemoryManager* oldVMM;
        LibC_Implementation::Heap* oldHeap;
    };
}