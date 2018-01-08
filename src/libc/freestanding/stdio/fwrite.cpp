#include <stdio.h>
#include "file.h"
#include <system_calls.h>

size_t fwrite(const void* restrict ptr, size_t size, size_t count,
    FILE* restrict stream) {
    
    if (size == 0 || count == 0) {
        return 0;
    }

    auto maxSizePerRequest = sizeof(VirtualFileSystem::WriteRequest::buffer);
    auto bytesWritten = 0;
    auto remainingBytes = size * count;
    auto buffer = static_cast<const unsigned char*>(ptr);

    while (remainingBytes > 0) {
        uint32_t length {0};

        if (maxSizePerRequest < remainingBytes) {
            length = maxSizePerRequest;
        }
        else {
            length = remainingBytes;
        }

        writeSynchronous(stream->descriptor, buffer + bytesWritten, length);
        bytesWritten += length;
        remainingBytes -= length;
    }

    return bytesWritten;    
}