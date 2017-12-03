#pragma once

#include "../object.h"
#include <string_view>

namespace HardwareFileSystem {

    class CPUTechnologyObject : public HardwareObject {
    public:

        CPUTechnologyObject();
        virtual ~CPUTechnologyObject() {}

        void readSelf(uint32_t requesterTaskId, uint32_t requestId) override;

        int getProperty(std::string_view name) override;
        void readProperty(uint32_t requesterTaskId, uint32_t requestId, uint32_t propertyId) override;
        void writeProperty(uint32_t requesterTaskId, uint32_t requestId, uint32_t propertyId, Vostok::ArgBuffer& args) override; 

        const char* getName() override;

    private:

        enum class PropertyId {
            EIST,
            TM2
        };

        uint32_t technology;
    };

    void detectTechnology(uint32_t ecx);
}