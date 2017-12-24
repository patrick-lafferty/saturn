#include <system_calls.h>
#include <services.h>
#include <stdio.h>

using namespace VirtualFileSystem;

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

    if (buffer.messageNamespace == IPC::MessageNamespace::VFS
            && buffer.messageId == static_cast<uint32_t>(MessageId::WriteResult)) {
        return IPC::extractMessage<WriteResult>(buffer);
    }
    else {
        asm("hlt");
        return {};
    }
}