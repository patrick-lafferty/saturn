#include <system_calls.h>
#include <services.h>
#include <stdio.h>

using namespace VirtualFileSystem;

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

    if (buffer.messageNamespace == IPC::MessageNamespace::VFS
            && buffer.messageId == static_cast<uint32_t>(MessageId::ReadResult)) {
        return IPC::extractMessage<ReadResult>(buffer);
    }
    else {
        asm ("hlt");
        return {};
    }
}

Read512Result read512Synchronous(uint32_t fileDescriptor, uint32_t length) {
    read(fileDescriptor, length);

    IPC::MaximumMessageBuffer buffer;
    receive(&buffer);

    if (buffer.messageNamespace == IPC::MessageNamespace::VFS
            && buffer.messageId == static_cast<uint32_t>(MessageId::Read512Result)) {
        return IPC::extractMessage<Read512Result>(buffer);
    }
    else {
        asm ("hlt");
        return {};
    }
}