#pragma once

#include "../object.h"
#include <string_view>

namespace HardwareFileSystem {

    class CPUExtensionsObject : public HardwareObject {
    public:

        CPUExtensionsObject();
        virtual ~CPUExtensionsObject() {}

        void readSelf(uint32_t requesterTaskId, uint32_t requestId) override;

        int getProperty(std::string_view name) override;
        void readProperty(uint32_t requesterTaskId, uint32_t requestId, uint32_t propertyId) override;
        void writeProperty(uint32_t requesterTaskId, uint32_t requestId, uint32_t propertyId, Vostok::ArgBuffer& args) override; 

        const char* getName() override;

    private:

        enum class PropertyId {
            VMX,
            SMX,
            FMA,
            FPU,
            VME,
            DE,
            PSE,
            PAE,
            MCE,
            PSE_36
        };

        uint32_t extensions;
    };

    void detectExtensions(uint32_t ecx, uint32_t edx);
}