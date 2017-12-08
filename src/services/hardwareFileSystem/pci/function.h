#pragma once

#include "../object.h"
#include <string_view>

namespace HardwareFileSystem::PCI {

    class FunctionObject : public HardwareObject {
    public:

        FunctionObject();
        virtual ~FunctionObject() {}

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

        bool exists() const;

        uint8_t getClassCode() const {return classCode;}
        uint8_t getSubClassCode() const {return subclassCode;}

    private:

        enum class FunctionId {
            Summary
        };

        void summary(uint32_t requesterTaskId, uint32_t requestId);

        enum class PropertyId {
            VendorId,
            ClassCode,
            SubclassCode,
            Interface,
            Bar0,
            Bar1,
            Bar2,
            Bar3,
            Bar4,
            Bar5,
            InterruptLine,
            InterruptPin
        };

        uint16_t vendorId;
        uint8_t classCode;
        uint8_t subclassCode;
        uint8_t interface;
        uint32_t bar0;
        uint32_t bar1;
        uint32_t bar2;
        uint32_t bar3;
        uint32_t bar4;
        uint32_t bar5;
        uint32_t interruptLine;
        uint32_t interruptPin;
    };
}