#pragma once

#include "../object.h"
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

        enum class PropertyId {
            VendorId,
            DeviceId,
            Bar0,
            Bar1,
            Bar2,
            Bar3,
            Bar4,
            Bar5
        };

        uint16_t vendorId;
        uint16_t deviceId;
        uint32_t bar0;
        uint32_t bar1;
        uint32_t bar2;
        uint32_t bar3;
        uint32_t bar4;
        uint32_t bar5;
    };
}