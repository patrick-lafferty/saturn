#include <system_calls.h>
#include <services/virtualFileSystem/virtualFileSystem.h>
#include <services.h>

void open(char* path) {
    VFS::OpenRequest open;
    open.serviceType = Kernel::ServiceType::VFS;
    memcpy(open.path, path, strlen(path));
    send(IPC::RecipientType::ServiceName, &open);
}

void read(uint32_t fileDescriptor, uint32_t length) {
    VFS::ReadRequest request;
    request.fileDescriptor = fileDescriptor;
    //request.length = length;
    request.serviceType = Kernel::ServiceType::VFS;

    send(IPC::RecipientType::ServiceName, &request);
}

void close(uint32_t fileDescriptor) {
    VFS::CloseRequest request;
    request.fileDescriptor = fileDescriptor;
    request.serviceType = Kernel::ServiceType::VFS;
    send(IPC::RecipientType::ServiceName, &request);
}