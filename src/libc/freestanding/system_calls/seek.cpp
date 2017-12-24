#include <system_calls.h>
#include <services.h>
#include <stdio.h>

using namespace VirtualFileSystem;

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

    if (buffer.messageNamespace == IPC::MessageNamespace::VFS
            && buffer.messageId == static_cast<uint32_t>(MessageId::SeekResult)) {
        return IPC::extractMessage<SeekResult>(buffer);
    }
    else {
        asm ("hlt");
        return {};
    }
}