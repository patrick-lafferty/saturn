#pragma once

#include <stdint.h>
#include <ipc.h>

namespace Kernel {

    enum class ServiceType {
        VGA,
        ServiceTypeEnd
    };

    struct RegisterService : IPC::Message {
        static uint32_t MessageId;
        ServiceType type;
    };

    struct RegisterServiceDenied : IPC::Message {
        static uint32_t MessageId;
        bool success {false};
    };

    struct ServiceMeta : IPC::Message {
    };

    struct VGAServiceMeta : ServiceMeta {
        static uint32_t MessageId;
        uint32_t vgaAddress {0};
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

    private:
        
        bool registerService(uint32_t taskId, ServiceType type);
        uint32_t getServiceTaskId(ServiceType type);

        void setupService(uint32_t taskId, ServiceType type);

        uint32_t* taskIds;
        ServiceMeta** meta;
    };
}