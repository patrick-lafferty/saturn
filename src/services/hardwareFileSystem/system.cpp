#include "system.h"
#include <services.h>
#include <system_calls.h>
#include <services/virtualFileSystem/virtualFileSystem.h>

using namespace Kernel;
using namespace VFS;

namespace HardwareFileSystem {

    void registerService() {
        MountRequest request;
        const char* path = "/system/hardware";
        memcpy(request.path, path, strlen(path) + 1);
        request.serviceType = ServiceType::VFS;

        send(IPC::RecipientType::ServiceName, &request);
    }

    void messageLoop() {

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