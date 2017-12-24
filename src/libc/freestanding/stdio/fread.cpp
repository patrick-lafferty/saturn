#include <stdio.h>
#include "file.h"
#include <system_calls.h>

size_t fread(void* restrict ptr, size_t size, size_t count, FILE* restrict stream) {

    if (size == 0 || count == 0) {
        return 0;
    }

    auto maxSizePerRequest = sizeof(VirtualFileSystem::Read512Result::buffer);
    auto bytesRead = 0;
    auto remainingBytes = size * count;
    auto buffer = static_cast<unsigned char*>(ptr);

    while (remainingBytes > 0) {
        uint32_t length {0};

        if (maxSizePerRequest < remainingBytes) {
            length = maxSizePerRequest;
        }
        else {
            length = remainingBytes;
        }

        auto result = read512Synchronous(stream->descriptor, length);

        if (result.success) {
            
            for (int i = 0; i < 2; i++) {

                memcpy(buffer + bytesRead, result.buffer, result.bytesWritten);
                bytesRead += result.bytesWritten;
                remainingBytes -= result.bytesWritten;
                stream->position += result.bytesWritten;

                if (result.expectMore) {
                    IPC::MaximumMessageBuffer messageBuffer;
                    receive(&messageBuffer);

                    if (messageBuffer.messageNamespace == IPC::MessageNamespace::VFS
                        && messageBuffer.messageId == static_cast<uint32_t>(VirtualFileSystem::MessageId::Read512Result)) {
                        result = IPC::extractMessage<VirtualFileSystem::Read512Result>(messageBuffer);
                    }
                }
                else {
                    break;
                }
            }
        }
        else {
            break;
        }
    }

    return bytesRead;
}