#include <system_calls.h>
#include <services.h>
#include <stdio.h>

using namespace VirtualFileSystem;

void close(uint32_t fileDescriptor) {
    CloseRequest request;
    request.fileDescriptor = fileDescriptor;
    request.serviceType = Kernel::ServiceType::VFS;
    send(IPC::RecipientType::ServiceName, &request);
}