#include <system_calls.h>
#include <services.h>
#include <stdio.h>

using namespace VirtualFileSystem;

void open(const char* path) {
    OpenRequest open;
    open.serviceType = Kernel::ServiceType::VFS;
    auto pathLength = strlen(path) + 1;
    memcpy(open.path, path, pathLength);
    open.shrink(pathLength);
    send(IPC::RecipientType::ServiceName, &open);
}

OpenResult openSynchronous(const char* path) {
    open(path);

    IPC::MaximumMessageBuffer buffer;
    receive(&buffer);

    if (buffer.messageId == OpenResult::MessageId) {
        return IPC::extractMessage<OpenResult>(buffer);
    }
    else {
        printf("[Syscall] openSynchronous: unexpected messageId: %d, expected: %d\n", buffer.messageId, OpenResult::MessageId);
        printf("    while opening path: %s\n", path);
        sleep(1000);
        asm ("hlt");
        return {};
    }
}

void create(const char* path) {
    CreateRequest request;
    request.serviceType = Kernel::ServiceType::VFS;
    auto pathLength = strlen(path) + 1;
    memcpy(request.path, path, pathLength);
    request.shrink(pathLength);
    send(IPC::RecipientType::ServiceName, &request);
}   

void read(uint32_t fileDescriptor, uint32_t length) {
    ReadRequest request;
    request.fileDescriptor = fileDescriptor;
    request.readLength = length;
    request.serviceType = Kernel::ServiceType::VFS;

    send(IPC::RecipientType::ServiceName, &request);
}

ReadResult readSynchronous(uint32_t fileDescriptor, uint32_t length) {
    read(fileDescriptor, length);

    IPC::MaximumMessageBuffer buffer;
    receive(&buffer);

    if (buffer.messageId == ReadResult::MessageId) {
        return IPC::extractMessage<ReadResult>(buffer);
    }
    else {
        asm ("hlt");
        return {};
    }
}

void write(uint32_t fileDescriptor, const void* data, uint32_t length) {
    WriteRequest request;
    request.fileDescriptor = fileDescriptor;
    request.writeLength = length;
    request.serviceType = Kernel::ServiceType::VFS;
    memcpy(request.buffer, data, length);
    send(IPC::RecipientType::ServiceName, &request);
}

VirtualFileSystem::WriteResult writeSynchronous(uint32_t fileDescriptor, const void* data, uint32_t length) {
    write(fileDescriptor, data, length);

    IPC::MaximumMessageBuffer buffer;
    receive(&buffer);

    if (buffer.messageId == WriteResult::MessageId) {
        return IPC::extractMessage<WriteResult>(buffer);
    }
    else {
        asm("hlt");
        return {};
    }
}

void close(uint32_t fileDescriptor) {
    CloseRequest request;
    request.fileDescriptor = fileDescriptor;
    request.serviceType = Kernel::ServiceType::VFS;
    send(IPC::RecipientType::ServiceName, &request);
}

void seek(uint32_t fileDescriptor, uint32_t offset, uint32_t origin) {
    SeekRequest request;
    request.serviceType = Kernel::ServiceType::VFS;
    request.fileDescriptor = fileDescriptor;
    request.offset = offset;
    request.origin = static_cast<Origin>(origin);
    send(IPC::RecipientType::ServiceName, &request);
}

VirtualFileSystem::SeekResult seekSynchronous(uint32_t fileDescriptor, uint32_t offset, uint32_t origin) {
    seek(fileDescriptor, offset, origin);

    IPC::MaximumMessageBuffer buffer;
    receive(&buffer);

    if (buffer.messageId == SeekResult::MessageId) {
        return IPC::extractMessage<SeekResult>(buffer);
    }
    else {
        asm ("hlt");
        return {};
    }
}