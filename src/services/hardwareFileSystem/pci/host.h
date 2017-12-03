#pragma once

#include "../object.h"
#include "device.h"
#include <string_view>

namespace HardwareFileSystem::PCI {

    class HostObject : public HardwareObject {
    public:

        HostObject();
        virtual ~HostObject() {}

        void readSelf(uint32_t requesterTaskId, uint32_t requestId) override;

        int getProperty(std::string_view name) override;
        void readProperty(uint32_t requesterTaskId, uint32_t requestId, uint32_t propertyId) override;
        void writeProperty(uint32_t requesterTaskId, uint32_t requestId, uint32_t propertyId, Vostok::ArgBuffer& args) override;

        Object* getNestedObject(std::string_view name) override;

        const char* getName() override;

    private:

        enum class FunctionId {

        };

        enum class PropertyId {
        };

        DeviceObject devices[32];
    };
}