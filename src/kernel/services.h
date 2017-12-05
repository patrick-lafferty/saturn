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

    struct RegisterService : IPC::Message {
        RegisterService() {
            messageId = MessageId;
            length = sizeof(RegisterService);
        }

        static uint32_t MessageId;
        ServiceType type;
    };

    typedef void (*PseudoMessageHandler)(IPC::Message*);

    struct RegisterPseudoService : IPC::Message {
        RegisterPseudoService() {
            messageId = MessageId;
            length = sizeof(RegisterPseudoService);
        }

        static uint32_t MessageId;
        ServiceType type;
        PseudoMessageHandler handler;
    };

    struct RegisterServiceDenied : IPC::Message {

        static uint32_t MessageId;
        bool success {false};
    };

    struct NotifyServiceReady : IPC::Message {
        NotifyServiceReady() {
            messageId = MessageId;
            length = sizeof(NotifyServiceReady);
        }

        static uint32_t MessageId;
    };

    struct ServiceMeta : IPC::Message {
        ServiceMeta() {
            ready = false;
        }

        bool ready;
    };

    struct VGAServiceMeta : ServiceMeta {
        VGAServiceMeta() { 
            messageId = MessageId; 
            length = sizeof(VGAServiceMeta);
        }

        static uint32_t MessageId;
        uint32_t vgaAddress {0};
    };

    struct GenericServiceMeta : ServiceMeta {
        GenericServiceMeta() {
            messageId = MessageId;
            length = sizeof(GenericServiceMeta);
        }

        static uint32_t MessageId;
    };

    struct SubscribeServiceRegistered : IPC::Message {
        SubscribeServiceRegistered() {
            messageId = MessageId;
            length = sizeof(SubscribeServiceRegistered);
        }

        static uint32_t MessageId;
        ServiceType type;
    };

    struct NotifyServiceRegistered : IPC::Message {
        NotifyServiceRegistered() {
            messageId = MessageId;
            length = sizeof(NotifyServiceRegistered);
        }

        static uint32_t MessageId;
        ServiceType type;
    };

    struct RunProgram : IPC::Message {
        RunProgram() {
            messageId = MessageId;
            length = sizeof(RunProgram);
        }

        static uint32_t MessageId;
        uintptr_t entryPoint;
    };

    struct RunResult : IPC::Message {
        RunResult() {
            messageId = MessageId;
            length = sizeof(RunResult);
        }

        static uint32_t MessageId;
        bool success;
        uint32_t pid;
    };

    struct LinearFrameBufferFound : IPC::Message {
        LinearFrameBufferFound() {
            messageId = MessageId;
            length = sizeof(LinearFrameBufferFound);
        }

        static uint32_t MessageId;
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

    private:
        
        bool registerService(uint32_t taskId, ServiceType type);
        bool registerPseudoService(ServiceType type, PseudoMessageHandler handler);
        void notifySubscribers(uint32_t index);
        void setupService(uint32_t taskId, ServiceType type);

        uint32_t* taskIds;
        std::vector<ServiceMeta> meta;
        PseudoMessageHandler* pseudoMessageHandlers;
        std::vector<std::vector<uint32_t>> subscribers;

        KnownHardwareAddresses addresses;
    };
}