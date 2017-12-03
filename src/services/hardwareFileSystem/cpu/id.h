#pragma once

#include "../object.h"
#include <string_view>

namespace HardwareFileSystem {
    class CPUIdObject : public HardwareObject {
    public:

        CPUIdObject();
        virtual ~CPUIdObject() {}

        void readSelf(uint32_t requesterTaskId, uint32_t requestId) override;

        int getProperty(std::string_view name) override;
        void readProperty(uint32_t requesterTaskId, uint32_t requestId, uint32_t propertyId) override;
        void writeProperty(uint32_t requesterTaskId, uint32_t requestId, uint32_t propertyId, Vostok::ArgBuffer& args) override; 

        const char* getName() override;

    private:

        enum class PropertyId {
            Model,
            FamilyId,
            Brand,    
        };
        
        uint32_t model;
        uint32_t familyId;
        char brand[48];
    };

    void detectId(uint32_t eax, uint32_t ebx);
}