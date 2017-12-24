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

    if (buffer.messageNamespace == IPC::MessageNamespace::VFS
            && buffer.messageId == static_cast<uint32_t>(MessageId::OpenResult)) {
        return IPC::extractMessage<OpenResult>(buffer);
    }
    else {
        printf("[Syscall] openSynchronous: unexpected messageId: %d, expected: %d\n", 
            buffer.messageId, MessageId::OpenResult);
        printf("    while opening path: %s\n", path);
        sleep(1000);
        asm ("hlt");
        return {};
    }
}