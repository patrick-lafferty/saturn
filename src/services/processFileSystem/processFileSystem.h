#pragma once

#include <stdint.h>
#include <ipc.h>
#include <services/virtualFileSystem/vostok.h>

namespace PFS {

    void service();

    class ProcessObject : public Vostok::Object {
    public:

        void read(uint32_t requesterTaskId, uint32_t functionId) override;
        int getFunction(char* name) override;
        void describe(uint32_t requesterTaskId, uint32_t functionId) override;

    private:

        void testA(uint32_t requesterTaskId);
        void testB(uint32_t requesterTaskId);

        enum class FunctionId {
            TestA,
            TestB
        };
    };
}