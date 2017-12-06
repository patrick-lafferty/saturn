#pragma once

#include "../object.h"
#include "function.h"
#include <string_view>

namespace HardwareFileSystem::PCI {

    class DeviceObject : public HardwareObject {
    public:

        DeviceObject();
        virtual ~DeviceObject() {}

        void readSelf(uint32_t requesterTaskId, uint32_t requestId) override;

        int getProperty(std::string_view name) override;
        void readProperty(uint32_t requesterTaskId, uint32_t requestId, uint32_t propertyId) override;
        void writeProperty(uint32_t requesterTaskId, uint32_t requestId, uint32_t propertyId, Vostok::ArgBuffer& args) override;

        Object* getNestedObject(std::string_view name) override;

        const char* getName() override;

        bool exists() const;

    private:

        enum class FunctionId {
        };

        void summary(uint32_t requesterTaskId, uint32_t requestId);

        enum class PropertyId {
            VendorId,
            DeviceId,
        };

        uint16_t vendorId;
        uint16_t deviceId;
        FunctionObject function[8];
    };
}