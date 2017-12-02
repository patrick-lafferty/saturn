#pragma once

#include "object.h"
#include <string_view>

namespace HardwareFileSystem {

    class CPUObject : public HardwareObject {
    public:

        CPUObject();
        virtual ~CPUObject() {}

        int getFunction(std::string_view name) override;
        void readFunction(uint32_t requesterTaskId, uint32_t requestId, uint32_t functionId) override;
        void writeFunction(uint32_t requesterTaskId, uint32_t requestId, uint32_t functionId, Vostok::ArgBuffer& args) override;
        void describeFunction(uint32_t requesterTaskId, uint32_t requestId, uint32_t functionId) override;

        int getProperty(std::string_view name) override;
        void readProperty(uint32_t requesterTaskId, uint32_t requestId, uint32_t propertyId) override;
        void writeProperty(uint32_t requesterTaskId, uint32_t requestId, uint32_t propertyId, Vostok::ArgBuffer& args) override;

        const char* getName() override;

    private:

        enum class FunctionId {

        };

        enum class PropertyId {
            test
        };

        char test[64];
    };

    void detectCPU();
}