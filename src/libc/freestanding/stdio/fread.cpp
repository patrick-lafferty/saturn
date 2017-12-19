#include <stdio.h>
#include "file.h"
#include <system_calls.h>

size_t fread(void* restrict ptr, size_t size, size_t count, FILE* restrict stream) {

    if (size == 0 || count == 0) {
        return 0;
    }

    auto maxSizePerRequest = sizeof(VirtualFileSystem::ReadResult::buffer);
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

        auto result = readSynchronous(stream->descriptor, length);

        if (result.success) {
            memcpy(buffer + bytesRead, result.buffer, result.bytesWritten);
            bytesRead += result.bytesWritten;
            remainingBytes -= length;
        }
        else {
            break;
        }
    }

    return bytesRead;
}