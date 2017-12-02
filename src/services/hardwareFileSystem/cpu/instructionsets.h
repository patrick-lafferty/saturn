#pragma once

#include "../object.h"
#include <string_view>

namespace HardwareFileSystem {

    class CPUInstructionSetsObject : public HardwareObject {
    public:

        CPUInstructionSetsObject();
        virtual ~CPUInstructionSetsObject() {}

        void readSelf(uint32_t requesterTaskId, uint32_t requestId) override;

        int getFunction(std::string_view name) override;
        void readFunction(uint32_t requesterTaskId, uint32_t requestId, uint32_t functionId) override;
        void writeFunction(uint32_t requesterTaskId, uint32_t requestId, uint32_t functionId, Vostok::ArgBuffer& args) override;
        void describeFunction(uint32_t requesterTaskId, uint32_t requestId, uint32_t functionId) override;

        int getProperty(std::string_view name) override;
        void readProperty(uint32_t requesterTaskId, uint32_t requestId, uint32_t propertyId) override;
        void writeProperty(uint32_t requesterTaskId, uint32_t requestId, uint32_t propertyId, Vostok::ArgBuffer& args) override; 

        Object* getNestedObject(std::string_view name) override;

        const char* getName() override;

    private:

        enum class PropertyId {
            SSE3,
            SSSE3,
            SSE4_1,
            SSE4_2,
            AVX
        };

        uint32_t features;
    };

    void detectInstructionSets(uint32_t ecx, uint32_t edx);

}