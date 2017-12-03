#pragma once

#include "../object.h"
#include "instructionsets.h"
#include "instructions.h"
#include "support.h"
#include "extensions.h"
#include "technology.h"
#include <string_view>

namespace HardwareFileSystem {

    class CPUFeaturesObject : public HardwareObject {
    public:

        CPUFeaturesObject();
        virtual ~CPUFeaturesObject() {}

        void readSelf(uint32_t requesterTaskId, uint32_t requestId) override;

        int getProperty(std::string_view name) override;
        void readProperty(uint32_t requesterTaskId, uint32_t requestId, uint32_t propertyId) override;
        void writeProperty(uint32_t requesterTaskId, uint32_t requestId, uint32_t propertyId, Vostok::ArgBuffer& args) override; 

        Object* getNestedObject(std::string_view name) override;

        const char* getName() override;

    private:

        enum class PropertyId {
            InstructionSets,
            Instructions,
            Features,
            Extensions,
            Technology
        };

        CPUInstructionSetsObject instructionSets;
        CPUInstructionsObject instructions;
        CPUSupportObject support;
        CPUExtensionsObject extensions;
        CPUTechnologyObject technology;
    };

    void detectFeatures(uint32_t ecx, uint32_t edx);
}