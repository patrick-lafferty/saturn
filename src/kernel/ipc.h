#pragma once

#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <stdio.h>

namespace Kernel {
    enum class ServiceType;
}

namespace IPC {

    struct Message {
        uint32_t length;
        uint32_t senderTaskId;
        union {
            uint32_t recipientId;
            Kernel::ServiceType serviceType;
        };
        uint32_t messageId;
    };

    inline constexpr uint32_t MaximumMessageSize {512};
    struct MaximumMessageBuffer : Message {
        uint8_t buffer[MaximumMessageSize];
    };

    enum class RecipientType {
        ServiceRegistryMailbox,
        ServiceName,
        TaskId
    };

    uint32_t getId(); 

    template<typename T>
    void registerMessage() {

        T::MessageId = getId();
    }

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

        void send(Message* message) {
            uint32_t availableSpace {0};

            if (lastReadOffset > lastWriteOffset) {
                availableSpace = lastReadOffset - lastWriteOffset;
            }
            else {
                availableSpace = bufferSize - lastWriteOffset + lastReadOffset;
            }

            if (message->length > availableSpace) {
                //??
                kprintf("[IPC] Send() ??\n");
            }
            else {
                auto ptr = buffer + lastWriteOffset;
                auto spaceUntilEnd = bufferSize - lastWriteOffset;

                if (message->length > spaceUntilEnd) {
                    //its cutoff
                    memcpy(ptr, message, spaceUntilEnd);
                    lastWriteOffset = 0;
                    ptr = buffer;
                    auto remainingMessageLength = message->length - spaceUntilEnd;
                    memcpy(ptr, reinterpret_cast<uint8_t*>(message) + spaceUntilEnd, remainingMessageLength);
                    lastWriteOffset = remainingMessageLength;
                }
                else {
                    memcpy(ptr, message, message->length);
                    lastWriteOffset += message->length;
                }

                unreadMessages++;
            }

             if (lastWriteOffset > bufferSize) {
                    kprintf("[Mailbox] lastWriteOffset is invalid\n");
                }
        }

        bool receive(Message* message) {
            if (unreadMessages == 0) {
                //block
                kprintf("[IPC] Read blocked\n");
                return false;
            }
            else {
                uint32_t messageLength {0};

                if ((bufferSize - lastReadOffset) < sizeof(Message)) {
                    auto spaceUntilEnd = bufferSize - lastReadOffset;
                    Message temp;

                    if (spaceUntilEnd > 0) {
                        memcpy(&temp, buffer + lastReadOffset, spaceUntilEnd);
                        memcpy(reinterpret_cast<uint8_t*>(&temp) + spaceUntilEnd, buffer, sizeof(Message) - spaceUntilEnd);
                    }
                    else {
                        memcpy(&temp, buffer, sizeof(Message));
                    }

                    messageLength = temp.length;
                }
                else {
                    messageLength = reinterpret_cast<Message*>(buffer + lastReadOffset)->length;
                }

                if (messageLength > 10000) {
                    kprintf("stop here");
                }

                auto ptr = buffer + lastReadOffset;

                if (lastReadOffset > lastWriteOffset) {
                    auto spaceUntilEnd = bufferSize - lastReadOffset;

                    if (messageLength > spaceUntilEnd) {
                        //its cutoff
                        memcpy(message, ptr, spaceUntilEnd);
                        lastReadOffset = 0;
                        ptr = buffer;
                        auto remainingMessageLength = messageLength - spaceUntilEnd;
                        memcpy(reinterpret_cast<uint8_t*>(message) + spaceUntilEnd, ptr, remainingMessageLength);
                        lastReadOffset = remainingMessageLength;   
                    }
                    else {
                        memcpy(message, ptr, messageLength);
                        lastReadOffset += messageLength;
                    }
                }
                else {
                    auto spaceUntilEnd = lastWriteOffset - lastReadOffset;

                    if (messageLength > spaceUntilEnd) {
                        memcpy(message, ptr, spaceUntilEnd);
                        lastReadOffset = 0;
                        ptr = buffer;
                        auto remainingMessageLength = messageLength - spaceUntilEnd;
                        memcpy(reinterpret_cast<uint8_t*>(message) + spaceUntilEnd, ptr, remainingMessageLength);
                        lastReadOffset = remainingMessageLength;
                    }
                    else {
                        memcpy(message, ptr, messageLength);
                        lastReadOffset += messageLength;
                    }
                }

                if (lastReadOffset > bufferSize) {
                    kprintf("[Mailbox] lastReadOffset is invalid\n");
                }

                unreadMessages--;
                return true;
            }
        }

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