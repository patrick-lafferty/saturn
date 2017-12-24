#pragma once

#include <stdint.h>
#include <ipc.h>
#include <vector>

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
        LinearFrameBufferFound
    };

    struct RegisterService : IPC::Message {
        RegisterService() {
            messageId = static_cast<uint32_t>(MessageId::RegisterService);
            length = sizeof(RegisterService);
            messageNamespace = static_cast<uint32_t>(IPC::MessageNamespace::ServiceRegistry);
        }

        ServiceType type;
    };

    enum class DriverType {
        ATA,
        DriverTypeEnd
    };

    struct RegisterDriver : IPC::Message {
        RegisterDriver() {
            messageId = static_cast<uint32_t>(MessageId::RegisterService);
            length = sizeof(RegisterDriver);
            messageNamespace = static_cast<uint32_t>(IPC::MessageNamespace::ServiceRegistry);
        }

        DriverType type;
    };

    struct RegisterDriverResult : IPC::Message {
        RegisterDriverResult() {
            messageId = static_cast<uint32_t>(MessageId::RegisterDriverResult);
            length = sizeof(RegisterDriverResult);
            messageNamespace = static_cast<uint32_t>(IPC::MessageNamespace::ServiceRegistry);
        }

        bool success;
    };

    struct DriverIrqReceived : IPC::Message {
        DriverIrqReceived() {
            messageId = static_cast<uint32_t>(MessageId::DriverIrqReceived);
            length = sizeof(DriverIrqReceived);
            messageNamespace = static_cast<uint32_t>(IPC::MessageNamespace::ServiceRegistry);
        }

    };

    typedef void (*PseudoMessageHandler)(IPC::Message*);

    struct RegisterPseudoService : IPC::Message {
        RegisterPseudoService() {
            messageId = static_cast<uint32_t>(MessageId::RegisterPseudoService);
            length = sizeof(RegisterPseudoService);
            messageNamespace = static_cast<uint32_t>(IPC::MessageNamespace::ServiceRegistry);
        }

        ServiceType type;
        PseudoMessageHandler handler;
    };

    struct RegisterServiceDenied : IPC::Message {
        RegisterServiceDenied() {
            messageId = static_cast<uint32_t>(MessageId::RegisterServiceDenied);
            length = sizeof(RegisterServiceDenied);
            messageNamespace = static_cast<uint32_t>(IPC::MessageNamespace::ServiceRegistry);
        }

        bool success {false};
    };

    struct NotifyServiceReady : IPC::Message {
        NotifyServiceReady() {
            messageId = static_cast<uint32_t>(MessageId::NotifyServiceReady);
            length = sizeof(NotifyServiceReady);
            messageNamespace = static_cast<uint32_t>(IPC::MessageNamespace::ServiceRegistry);
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
            messageNamespace = static_cast<uint32_t>(IPC::MessageNamespace::ServiceRegistry);
        }

        uint32_t vgaAddress {0};
    };

    struct GenericServiceMeta : ServiceMeta {
        GenericServiceMeta() {
            messageId = static_cast<uint32_t>(MessageId::GenericServiceMeta);
            length = sizeof(GenericServiceMeta);
            messageNamespace = static_cast<uint32_t>(IPC::MessageNamespace::ServiceRegistry);
        }

    };

    struct SubscribeServiceRegistered : IPC::Message {
        SubscribeServiceRegistered() {
            messageId = static_cast<uint32_t>(MessageId::SubscribeServiceRegistered);
            length = sizeof(SubscribeServiceRegistered);
            messageNamespace = static_cast<uint32_t>(IPC::MessageNamespace::ServiceRegistry);
        }

        ServiceType type;
    };

    struct NotifyServiceRegistered : IPC::Message {
        NotifyServiceRegistered() {
            messageId = static_cast<uint32_t>(MessageId::NotifyServiceRegistered);
            length = sizeof(NotifyServiceRegistered);
            messageNamespace = static_cast<uint32_t>(IPC::MessageNamespace::ServiceRegistry);
        }

        ServiceType type;
    };

    struct RunProgram : IPC::Message {
        RunProgram() {
            messageId = static_cast<uint32_t>(MessageId::RunProgram);
            length = sizeof(RunProgram);
            messageNamespace = static_cast<uint32_t>(IPC::MessageNamespace::ServiceRegistry);
        }

        uintptr_t entryPoint;
    };

    struct RunResult : IPC::Message {
        RunResult() {
            messageId = static_cast<uint32_t>(MessageId::RunResult);
            length = sizeof(RunResult);
            messageNamespace = static_cast<uint32_t>(IPC::MessageNamespace::ServiceRegistry);
        }

        bool success;
        uint32_t pid;
    };

    struct LinearFrameBufferFound : IPC::Message {
        LinearFrameBufferFound() {
            messageId = static_cast<uint32_t>(MessageId::LinearFrameBufferFound);
            length = sizeof(LinearFrameBufferFound);
            messageNamespace = static_cast<uint32_t>(IPC::MessageNamespace::ServiceRegistry);
        }

        uint32_t address;
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
        
        void receiveMessage(IPC::Message* message);
        void receivePseudoMessage(ServiceType type, IPC::Message* message);
        uint32_t getServiceTaskId(ServiceType type);
        bool isPseudoService(ServiceType type);
        bool handleDriverIrq(uint32_t irq);

    private:
        
        bool registerService(uint32_t taskId, ServiceType type);
        bool registerDriver(uint32_t taskId, DriverType type);
        bool registerPseudoService(ServiceType type, PseudoMessageHandler handler);
        void notifySubscribers(uint32_t index);
        void setupService(uint32_t taskId, ServiceType type);
        void setupDriver(uint32_t taskId, DriverType type);

        uint32_t* taskIds;
        std::vector<ServiceMeta> meta;
        PseudoMessageHandler* pseudoMessageHandlers;
        std::vector<std::vector<uint32_t>> subscribers;
        uint32_t* driverTaskIds;

        KnownHardwareAddresses addresses;
    };
}