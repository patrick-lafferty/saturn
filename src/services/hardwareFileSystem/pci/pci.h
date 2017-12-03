#pragma once

#include "../object.h"
#include "host.h"
#include <string_view>

namespace HardwareFileSystem::PCI {

    class Object : public HardwareObject {
    public:

        Object();
        virtual ~Object() {}

        void readSelf(uint32_t requesterTaskId, uint32_t requestId) override;

        int getProperty(std::string_view name) override;
        void readProperty(uint32_t requesterTaskId, uint32_t requestId, uint32_t propertyId) override;
        void writeProperty(uint32_t requesterTaskId, uint32_t requestId, uint32_t propertyId, Vostok::ArgBuffer& args) override;

        Vostok::Object* getNestedObject(std::string_view name) override;

        const char* getName() override;

    private:

        enum class FunctionId {

        };

        enum class PropertyId {
            Host0
        };

        HostObject host0;
    };
}