#pragma once

#include <stdint.h>
#include <services/virtualFileSystem/vostok.h>

namespace PFS {

    /*
    A ProcessObject is meant to emulate entries in Plan9's /proc
    It allows the user/other processes to query data about
    the process, such as the executable it started,
    or the commandline args perhaps
    */
    class ProcessObject : public Vostok::Object {
    public:

        ProcessObject(uint32_t pid = 0);
        virtual ~ProcessObject() {}

        int getFunction(std::string_view name) override;
        void readFunction(uint32_t requesterTaskId, uint32_t requestId, uint32_t functionId) override;
        void writeFunction(uint32_t requesterTaskId, uint32_t requestId, uint32_t functionId, Vostok::ArgBuffer& args) override;
        void describeFunction(uint32_t requesterTaskId, uint32_t requestId, uint32_t functionId) override;

        int getProperty(std::string_view name) override;
        void readProperty(uint32_t requesterTaskId, uint32_t requestId, uint32_t propertyId) override;
        void writeProperty(uint32_t requesterTaskId, uint32_t requestId, uint32_t propertyId, Vostok::ArgBuffer& args) override;

        uint32_t pid;

    private:

        void testA(uint32_t requesterTaskId, int x);
        void testB(uint32_t requesterTaskId, bool b);

        /*
        The convention for VostokObjects is to have two enums that
        represent the publicly exposed functions and properties
        accessible via the VFS. getFunction/getProperty takes a 
        string name and sees if it matches something in these enums.
        */
        enum class FunctionId {
            TestA,
            TestB
        };

        enum class PropertyId {
            Executable
        };

        char executable[256];

    };
}