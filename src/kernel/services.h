#pragma once

#include <stdint.h>

namespace Kernel {

    enum class ServiceType {
        VGA,
        ServiceTypeEnd
    };

    class ServiceRegistry {
    public:
        
        ServiceRegistry();
        
        bool registerService(uint32_t taskId, ServiceType type);
        uint32_t getServiceTaskId(ServiceType type);

    private:

        void setupService(uint32_t taskId, ServiceType type);

        uint32_t* taskIds;
    };
}