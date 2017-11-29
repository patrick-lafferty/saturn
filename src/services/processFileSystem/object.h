#pragma once

#include <stdint.h>
#include <services/virtualFileSystem/vostok.h>

namespace PFS {

    class ProcessObject : public Vostok::Object {
    public:

        ProcessObject();

        int getFunction(char* name) override;
        void readFunction(uint32_t requesterTaskId, uint32_t functionId) override;
        void writeFunction(uint32_t requesterTaskId, uint32_t functionId, Vostok::ArgBuffer& args) override;
        void describeFunction(uint32_t requesterTaskId, uint32_t functionId) override;

        int getProperty(char* name) override;
        void readProperty(uint32_t requesterTaskId, uint32_t propertyId) override;
        void writeProperty(uint32_t requesterTaskId, uint32_t propertyId, Vostok::ArgBuffer& args) override;

    private:

        void testA(uint32_t requesterTaskId, int x);
        void testB(uint32_t requesterTaskId, bool b);

        enum class FunctionId {
            TestA,
            TestB
        };

        enum class PropertyId {
            Foo,
            Bar
        };

        char foo[10];
        uint32_t bar;
    };
}