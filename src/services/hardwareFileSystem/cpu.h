#pragma once

#include <services/virtualFileSystem/vostok.h>

namespace HardwareFileSystem {

    class CPUObject : public Vostok::Object {
    public:

        CPUObject();
        virtual ~CPUObject() {}

        int getFunction(char* name) override;
        void readFunction(uint32_t requesterTaskId, uint32_t functionId) override;
        void writeFunction(uint32_t requesterTaskId, uint32_t functionId, Vostok::ArgBuffer& args) override;
        void describeFunction(uint32_t requesterTaskId, uint32_t functionId) override;

        int getProperty(char* name) override;
        void readProperty(uint32_t requesterTaskId, uint32_t propertyId) override;
        void writeProperty(uint32_t requesterTaskId, uint32_t propertyId, Vostok::ArgBuffer& args) override;

    private:
    };
}