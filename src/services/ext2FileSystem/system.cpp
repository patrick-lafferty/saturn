#include "system.h"
#include <services.h>
#include <system_calls.h>
#include <services/virtualFileSystem/virtualFileSystem.h>
#include <vector>
#include <parsing>

using namespace Kernel;
using namespace VFS;

namespace Ext2FileSystem {

    void registerService() {
        MountRequest request;
        const char* path = "/test";
        memcpy(request.path, path, strlen(path) + 1);
        request.serviceType = ServiceType::VFS;

        send(IPC::RecipientType::ServiceName, &request);
    }

    void messageLoop() {

        printf("Ext2FS started\n");

        while (true) {
            IPC::MaximumMessageBuffer buffer;
            receive(&buffer);

        }
    }

    void service() {
        waitForServiceRegistered(ServiceType::VFS);
        registerService();
        messageLoop();
    }
}