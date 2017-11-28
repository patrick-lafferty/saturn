#pragma once

#include <stdint.h>
#include <ipc.h>

namespace Kernel {

    enum class ServiceType {
        VGA,
        Terminal,
        PS2,
        Keyboard,
        Mouse,
        Memory,
        VFS,
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

    struct ServiceMeta : IPC::Message {
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

    template<typename T>
    class Array {
    public:
        
        Array() {
            maxItems = 10;
            data = new T[maxItems];
        }
        
        void add(T item) {
            if (length >= maxItems) {
                auto oldSize = maxItems;
                maxItems *= 2;
                auto newData = new T[maxItems];
                memcpy(newData, data, sizeof(T) * oldSize);
                delete data;
                data = newData;
            }

            data[length] = item;
            length++;
        }

        T* begin() {
            return data;
        }

        T* end() {
            return data + length;
        }

    private:
        uint32_t length {0};
        uint32_t maxItems {0};
        T* data {nullptr};
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

        void setupService(uint32_t taskId, ServiceType type);

        uint32_t* taskIds;
        ServiceMeta** meta;
        PseudoMessageHandler* pseudoMessageHandlers;
        Array<uint32_t>* subscribers;
    };
}