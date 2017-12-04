#include <system_calls.h>
#include <services.h>

void open(const char* path) {
    VFS::OpenRequest open;
    open.serviceType = Kernel::ServiceType::VFS;
    auto pathLength = strlen(path) + 1;
    memcpy(open.path, path, pathLength);
    open.shrink(pathLength);
    send(IPC::RecipientType::ServiceName, &open);
}

VFS::OpenResult openSynchronous(const char* path) {
    open(path);

    IPC::MaximumMessageBuffer buffer;
    receive(&buffer);

    if (buffer.messageId == VFS::OpenResult::MessageId) {
        return IPC::extractMessage<VFS::OpenResult>(buffer);
    }
    else {
        asm ("hlt");
        return {};
    }
}

void create(const char* path) {
    VFS::CreateRequest request;
    request.serviceType = Kernel::ServiceType::VFS;
    auto pathLength = strlen(path) + 1;
    memcpy(request.path, path, pathLength);
    request.shrink(pathLength);
    send(IPC::RecipientType::ServiceName, &request);
}   

void read(uint32_t fileDescriptor, uint32_t length) {
    VFS::ReadRequest request;
    request.fileDescriptor = fileDescriptor;
    request.readLength = length;
    request.serviceType = Kernel::ServiceType::VFS;

    send(IPC::RecipientType::ServiceName, &request);
}

VFS::ReadResult readSynchronous(uint32_t fileDescriptor, uint32_t length) {
    read(fileDescriptor, length);

    IPC::MaximumMessageBuffer buffer;
    receive(&buffer);

    if (buffer.messageId == VFS::ReadResult::MessageId) {
        return IPC::extractMessage<VFS::ReadResult>(buffer);
    }
    else {
        asm ("hlt");
        return {};
    }
}

void write(uint32_t fileDescriptor, const void* data, uint32_t length) {
    VFS::WriteRequest request;
    request.fileDescriptor = fileDescriptor;
    request.writeLength = length;
    request.serviceType = Kernel::ServiceType::VFS;
    memcpy(request.buffer, data, length);
    send(IPC::RecipientType::ServiceName, &request);
}

void close(uint32_t fileDescriptor) {
    VFS::CloseRequest request;
    request.fileDescriptor = fileDescriptor;
    request.serviceType = Kernel::ServiceType::VFS;
    send(IPC::RecipientType::ServiceName, &request);
}