#pragma once

#include <stdint.h>
#include <services/virtualFileSystem/vostok.h>

namespace PFS {

    class ProcessObject : public Vostok::Object {
    public:

        void read(uint32_t requesterTaskId, uint32_t functionId) override;
        void write(uint32_t requesterTaskId, uint32_t functionId, Vostok::ArgBuffer& args) override;
        int getFunction(char* name) override;
        void describe(uint32_t requesterTaskId, uint32_t functionId) override;

    private:

        void testA(uint32_t requesterTaskId, int x);
        void testB(uint32_t requesterTaskId, bool b);

        enum class FunctionId {
            TestA,
            TestB
        };
    };
}