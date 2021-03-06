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
        Mouse,
        Memory,
        VFS,
        WindowManager
    };

    /*
    Base class for all messages. Stores the message's header
    that indicates the size of the message, who sent it and
    who will receive it, as well as how to identify the message
    */
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
        TaskId
    };

    /*
    Helper func that copies the first n bytes from the buffer
    to create a T message
    */
    template<typename T>
    T extractMessage(const MaximumMessageBuffer& buffer) {
        T result;

        memcpy(&result, &buffer, sizeof(T));

        return result;
    }

    struct StoredMessage {
        MaximumMessageBuffer buffer;
        StoredMessage* next {nullptr};
    };

    /*
    A Mailbox stores and provides access to messages. Each task
    has one mailbox. Messages are stored in a circular byte buffer.
    */
    class Mailbox {
    public:

        Mailbox(uint32_t bufferAddress, uint32_t size) {
            //buffers = reinterpret_cast<MaximumMessageBuffer*>(bufferAddress);
            messages = reinterpret_cast<StoredMessage*>(bufferAddress);
            bufferSize = size;
        }

        /*
        Copies the message into the buffer if there is space available,
        overwriting any messages that were previously read
        */
        void send(Message* message);     

        /*
        Extracts a message from the buffer and copies it to the supplied message
        */
        bool receive(Message* message);

        /*
        Searches for a message with the given namespace and messageId
        Copies the message to message buffer and returns true if found,
        false otherwise
        */
        bool filteredReceive(Message* message, IPC::MessageNamespace filter, uint32_t messageId);  

        bool hasUnreadMessages() {
            return unreadMessages > 0;
        }

        uint32_t getUnreadMessagesCount() const {
            return unreadMessages;
        }

    private:

        uint32_t unreadMessages {0};
        uint32_t bufferSize {0};
        uint32_t lastReadOffset {0};
        uint32_t lastWriteOffset {0};
        uint32_t bufferLock {0};
        //MaximumMessageBuffer* buffers;
        StoredMessage* messages;
        StoredMessage* firstMessage {nullptr};
        StoredMessage* lastMessage {nullptr};
        int freeMessages {70};
        int MaxMessages {70};
    };
}
