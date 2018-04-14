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
#include "vostok.h"
#include <string.h>
#include "messages.h"
#include <system_calls.h>

namespace Vostok {
    template<>
    void ArgBuffer::write(char* arg, ArgTypes type) {
        if (typedBuffer[nextTypeIndex] == static_cast<uint8_t>(ArgTypes::EndArg)
            || static_cast<uint8_t>(type) != typedBuffer[nextTypeIndex]) {
            writeFailed = true;
        }
        else {
            nextTypeIndex--;
            auto size = strlen(arg) + 1;
            memcpy(typedBuffer + nextValueIndex, arg, size);
            nextValueIndex += size;
        }
    }

    template<>
    void ArgBuffer::write(const char* arg, ArgTypes type) {
        write(const_cast<char*>(arg), type);
    }

    template<>
    void ArgBuffer::writeValueWithType(char* arg, ArgTypes type) {
        writeType(type);
        nextTypeIndex++;
        write(arg, type);
    }

    template<>
    void ArgBuffer::writeValueWithType(const char* arg, ArgTypes type) {
        writeValueWithType(const_cast<char*>(arg), type);
    }

    template<>
    char* ArgBuffer::read(ArgTypes type) {
        if (peekType() != type) {
            readFailed = true;
            return {};
        }
        else {
            auto result = reinterpret_cast<const char*>(typedBuffer + nextValueIndex);
            auto length = strlen(result) + 1;
            nextValueIndex += length;
            nextTypeIndex--;
            return const_cast<char*>(result);
        }
    }

    template<>
    const char* ArgBuffer::read(ArgTypes type) {
        return const_cast<const char*>(read<char*>(type));
    }

    void replyWriteSucceeded(uint32_t requesterTaskId, uint32_t requestId, bool success, bool expectReadResult) {
        VirtualFileSystem::WriteResult result;
        result.requestId = requestId;
        result.success = success;
        result.recipientId = requesterTaskId;
        result.expectReadResult = expectReadResult;
        send(IPC::RecipientType::TaskId, &result);
    }
}