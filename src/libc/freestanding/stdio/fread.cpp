/*
Copyright (c) 2017, Patrick Lafferty
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

    * Redistributions of source code must retain the above copyright
      notice, this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright
      notice, this list of conditions and the following disclaimer in the
      documentation and/or other materials provided with the distribution.
    * Neither the name of the copyright holder nor the names of its 
      contributors may be used to endorse or promote products derived from 
      this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR 
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/
#include <stdio.h>
#include "file.h"
#include <system_calls.h>
#include <services.h>

using namespace VirtualFileSystem;

size_t fread(void* restrict ptr, size_t size, size_t count, FILE* restrict stream) {

    if (size == 0 || count == 0) {
        return 0;
    }

    auto maxSizePerRequest = sizeof(Read512Result::buffer);
    auto bytesRead = 0;
    auto remainingBytes = size * count;
    auto buffer = static_cast<unsigned char*>(ptr);

    if (remainingBytes > 4096) {

        ReadStreamRequest streamRequest;
        streamRequest.fileDescriptor = stream->descriptor;
        streamRequest.readLength = remainingBytes;
        streamRequest.serviceType = Kernel::ServiceType::VFS;

        send(IPC::RecipientType::ServiceName, &streamRequest);

        Kernel::ShareMemoryRequest shareRequest;
        shareRequest.ownerAddress = reinterpret_cast<uintptr_t>(ptr);
        shareRequest.sharedServiceType = Kernel::ServiceType::VFS;
        shareRequest.recipientIsTaskId = false;
        shareRequest.size = remainingBytes;

        //make sure that all the pages have been allocated before sharing them
        memset(ptr, 0, remainingBytes);

        send(IPC::RecipientType::ServiceRegistryMailbox, &shareRequest);

        {
            IPC::MaximumMessageBuffer buffer;
            filteredReceive(&buffer, 
                IPC::MessageNamespace::ServiceRegistry,
                static_cast<uint32_t>(Kernel::MessageId::ShareMemoryResult));
        }

        {
            IPC::MaximumMessageBuffer buffer;
            filteredReceive(&buffer,
                IPC::MessageNamespace::VFS,
                static_cast<uint32_t>(MessageId::ReadStreamResult));

            auto result = IPC::extractMessage<ReadStreamResult>(buffer);

            if (result.success) {
                //TODO: stop sharing
                return result.bytesWritten;
            }
            else {
                return 0;
            }
        }

    }
    else {

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
                        filteredReceive(&messageBuffer, IPC::MessageNamespace::VFS, static_cast<uint32_t>(MessageId::Read512Result));

                        result = IPC::extractMessage<Read512Result>(messageBuffer);
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
    }

    return bytesRead;
}