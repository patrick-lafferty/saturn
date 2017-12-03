#pragma once

#include "../object.h"
#include <string_view>

namespace HardwareFileSystem {

    class CPUSupportObject : public HardwareObject {
    public:

        CPUSupportObject();
        virtual ~CPUSupportObject() {}

        void readSelf(uint32_t requesterTaskId, uint32_t requestId) override;

        int getProperty(std::string_view name) override;
        void readProperty(uint32_t requesterTaskId, uint32_t requestId, uint32_t propertyId) override;
        void writeProperty(uint32_t requesterTaskId, uint32_t requestId, uint32_t propertyId, Vostok::ArgBuffer& args) override; 

        const char* getName() override;

    private:

        enum class PropertyId {
            DTES64,
            MONITOR,
            DS_CPL,
            CNXT_ID,
            SDBG,
            XTPR,
            PDCM,
            PCID,
            DCA,
            X2APIC,
            TSC_DEADLINE,
            MTRR,
            PGE,
            PAT,
            PSN,
            APIC,
        };

        uint32_t support;
    };

    void detectSupport(uint32_t ecx, uint32_t edx);
}