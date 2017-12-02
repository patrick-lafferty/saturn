#pragma once

#include "../object.h"
#include "instructionsets.h"
#include "instructions.h"
#include <string_view>

namespace HardwareFileSystem {

    class CPUFeaturesObject : public HardwareObject {
    public:

        CPUFeaturesObject();
        virtual ~CPUFeaturesObject() {}

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
            InstructionSets,
            Instructions,
            Features,
            Extensions,
            Technology
        };

        CPUInstructionSetsObject instructionSets;
        CPUInstructionsObject instructions;

        /*

        instructions
        pclmulqdq
        cmpxchg16b
        movbe
        popcnt
        aesni
        xsave
        osxsave
        f16c
        rdrand
        tsc
        msr
        cmpxchg8b
        sep
        cmov
        clfsh

        features
        dtes64
        monitor
        ds_cpl
        cnxt_id
        sdbg
        xtpr
        pdcm
        pcid
        dca
        x2apic
        tsc_deadline
        mtrr
        pge
        pat
        psn
        apic

        extensions
        vmx
        smx
        fma
        fpu
        vme
        de
        pse
        pae
        mce
        pse_36

        technology
        eist
        tm2

        ??
        */
    };

    void detectFeatures(uint32_t ecx, uint32_t edx);
}