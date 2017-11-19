#pragma once

#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <stdio.h>

namespace IPC {

    struct Message {
        uint32_t length;
    };

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
                printf("[IPC] Send() ??\n");
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
                    memcpy(ptr, message + spaceUntilEnd, remainingMessageLength);
                    lastWriteOffset = remainingMessageLength;
                }
                else {
                    memcpy(ptr, message, message->length);
                    lastWriteOffset += message->length;
                }

                unreadMessages++;
            }

            //printf("[IPC] Send() lastRead: %d, lastWrite: %d\n", lastReadOffset, lastWriteOffset);
        }

        bool receive(Message* message) {
            if (unreadMessages == 0) {
                //block
                return false;
            }
            else {
                auto messageLength = reinterpret_cast<Message*>(buffer + lastReadOffset)->length;
                //printf("[IPC] Receive() messageLength: %d\n", messageLength);
                auto ptr = buffer + lastReadOffset;

                if (lastReadOffset > lastWriteOffset) {
                    auto spaceUntilEnd = bufferSize - lastWriteOffset;

                    if (messageLength> spaceUntilEnd) {
                        //its cutoff
                        memcpy(message, ptr, spaceUntilEnd);
                        lastReadOffset = 0;
                        ptr = buffer;
                        auto remainingMessageLength = messageLength - spaceUntilEnd;
                        memcpy(message + spaceUntilEnd, ptr, remainingMessageLength);
                        lastReadOffset = remainingMessageLength;   
                    }
                    else {
                        memcpy(message, ptr, messageLength);
                        lastReadOffset += messageLength;
                    }
                }
                else {
                    memcpy(message, ptr, messageLength);
                    lastReadOffset += messageLength;
                }

                unreadMessages--;
                //printf("[IPC] Read() lastRead: %d, lastWrite: %d\n", lastReadOffset, lastWriteOffset);
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