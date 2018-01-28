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
#pragma once

#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <stdio.h>

namespace Kernel {
    enum class ServiceType;
}

namespace IPC {

    enum class MessageNamespace : uint32_t {
        ServiceRegistry,
        Scheduler,
        VGA,
        Terminal,
        PS2,
        Keyboard,
        Memory,
        VFS,
        WindowManager
    };

    struct Message {
        uint32_t length;
        uint32_t senderTaskId;

        union {
            uint32_t recipientId;
            Kernel::ServiceType serviceType;
        };

        MessageNamespace messageNamespace;
        uint32_t messageId;
    };

    inline constexpr uint32_t MaximumMessageSize {1024};
    struct MaximumMessageBuffer : Message {
        uint8_t buffer[MaximumMessageSize];
    };

    enum class RecipientType {
        ServiceRegistryMailbox,
        ServiceName,
        Scheduler,
        TaskId
    };

    template<typename T>
    T extractMessage(const MaximumMessageBuffer& buffer) {
        T result;

        memcpy(&result, &buffer, sizeof(T));

        return result;
    }

    class Mailbox {
    public:

        Mailbox(uint32_t bufferAddress, uint32_t size) {
            buffer = reinterpret_cast<uint8_t*>(bufferAddress);
            bufferSize = size;
        }

        void send(Message* message);     

        bool receive(Message* message);
        //bool receive(Message* message, MessageNamespace filter, uint32_t messageId);

        bool hasUnreadMessages() {
            return unreadMessages > 0;
        }

    private:

        uint32_t unreadMessages {0};
        uint32_t bufferSize {0};
        uint8_t* buffer {nullptr};
        uint32_t lastReadOffset {0};
        uint32_t lastWriteOffset {0};

    };
}