#include <system_calls.h>
#include <services.h>
#include <stdio.h>

using namespace VirtualFileSystem;

void create(const char* path) {
    CreateRequest request;
    request.serviceType = Kernel::ServiceType::VFS;
    auto pathLength = strlen(path) + 1;
    memcpy(request.path, path, pathLength);
    request.shrink(pathLength);
    send(IPC::RecipientType::ServiceName, &request);
}  