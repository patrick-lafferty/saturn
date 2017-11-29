#pragma once

#include <stdint.h>
#include <ipc.h>
#include <services/virtualFileSystem/virtualFileSystem.h>

namespace PFS {

    void service();

    class ProcessObject : public VFS::Object {
    public:

        void read(uint32_t requesterTaskId, uint32_t functionId) override;
        int getFunction(char* name) override;

    private:

        void testA(uint32_t requesterTaskId);
        void testB(uint32_t requesterTaskId);

        enum class FunctionId {
            TestA,
            TestB
        };
    };
}