#pragma once

#include "../object.h"
#include "id.h"
#include "features.h"
#include <string_view>

namespace HardwareFileSystem {

    class CPUObject : public HardwareObject {
    public:

        CPUObject();
        virtual ~CPUObject() {}

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
            Id,
            Features
        };

        CPUIdObject id;
        CPUFeaturesObject features;
    };

    void detectCPU();
}