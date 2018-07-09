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
#include <ipc.h>
#include <locks.h>

namespace IPC {

    void Mailbox::send(Message* message) {

        Kernel::SpinLock lock {&bufferLock};
        sendLockless(message);
    }

    void Mailbox::sendLockless(Message* message) {
        if (freeMessages > 0) {
            for (auto i = 0; i < MaxMessages; i++) {
                if (buffers[i].length == 0) {
                    memcpy(&buffers[i], message, message->length);
                    break;
                }
            }

            freeMessages--;
            unreadMessages++;
        }
        else {
            asm("hlt");
        }
    }

    #if 0

    void Mailbox::sendLockless(Message* message) {
        uint32_t availableSpace {0};

        if (lastReadOffset > lastWriteOffset) {
            availableSpace = lastReadOffset - lastWriteOffset;
        }
        else {
            availableSpace = bufferSize - lastWriteOffset + lastReadOffset;
        }

        if (message->length > availableSpace) {
            if (lastReadOffset >= lastWriteOffset
                && unreadMessages > 0) {
                auto nextFreeSpot = findEndReadOffset();

                if (nextFreeSpot > lastReadOffset) {
                    availableSpace = bufferSize - nextFreeSpot + lastWriteOffset;
                }
                else {
                    availableSpace = lastWriteOffset - nextFreeSpot;
                }

                if (message->length > availableSpace) {
                    asm("hlt");
                }

                markSkippableRange(lastWriteOffset, lastReadOffset - lastWriteOffset);
                lastWriteOffset = nextFreeSpot;
            }
            else {
                asm("hlt");
            }
        }

        auto ptr = buffer + lastWriteOffset;

        if (lastWriteOffset >= lastReadOffset) {
            auto remainingSpace = bufferSize - lastWriteOffset;

            if (remainingSpace < message->length) {
                memcpy(ptr, message, remainingSpace);
                auto remainingBytes = message->length - remainingSpace;
                memcpy(buffer, 
                    reinterpret_cast<uint8_t*>(message) + remainingSpace, 
                    remainingBytes);
                lastWriteOffset = remainingBytes;
            }
            else {
                memcpy(ptr, message, message->length);
                lastWriteOffset += message->length;
            }
        }
        else {
            memcpy(ptr, message, message->length);
            lastWriteOffset += message->length;
        }

        unreadMessages++;
    }

    #if 0
    void Mailbox::sendLockless(Message* message) {

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
            asm("hlt");
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
            asm("hlt");
        }
    }
    #endif
    #endif

    bool Mailbox::receive(Message* message) {

        Kernel::SpinLock lock {&bufferLock};

        return receiveLockless(message);
    }

    bool Mailbox::receiveLockless(Message* message) {
        if (unreadMessages == 0) {
            return false;
        }

        for (auto i = 0; i < MaxMessages; i++) {
            if (buffers[i].length > 0) {
                memcpy(message, &buffers[i], buffers[i].length);
                unreadMessages--;
                freeMessages++;
                buffers[i].length = 0;
                return true;
            }
        }

        return false;
    }

    bool Mailbox::filteredReceiveLockless(Message* message, IPC::MessageNamespace filter, uint32_t messageId) {
        if (unreadMessages == 0) {
            return false;
        }

        for (auto i = 0; i < MaxMessages; i++) {
            if (buffers[i].length > 0
                && buffers[i].messageNamespace == filter
                && buffers[i].messageId == messageId) {
                memcpy(message, &buffers[i], buffers[i].length);
                unreadMessages--;
                freeMessages++;
                buffers[i].length = 0;
                return true;
            }
        }

        return false;
    }

    #if 0

        auto messageLength = *reinterpret_cast<uint32_t*>(buffer + lastReadOffset);
        auto messageStart = lastReadOffset;

        if (lastReadOffset >= lastWriteOffset) {
            auto remainingSpace = bufferSize - lastReadOffset;

            bool skipped {false};

            while (canSkipRead(lastReadOffset, messageLength)) {
                skipRead(message, messageLength);
                messageLength = *reinterpret_cast<uint32_t*>(buffer + lastReadOffset);
                skipped = true;
            }

            if (skipped) {
                return receiveLockless(message);
            }

            if (messageLength > remainingSpace) {
                memcpy(message, buffer + lastReadOffset, remainingSpace);
                auto remainingBytes = messageLength - remainingSpace;
                memcpy(reinterpret_cast<uint8_t*>(message) + remainingSpace, 
                    buffer, remainingBytes);
                lastReadOffset = remainingBytes;
            }
            else {
                memcpy(message, buffer + lastReadOffset, messageLength);
            }
        }
        else {
            auto remainingSpace = lastWriteOffset - lastReadOffset;

            bool skipped {false};
            while (canSkipRead(lastReadOffset, messageLength)) {
                skipRead(message, messageLength);
                messageLength = *reinterpret_cast<uint32_t*>(buffer + lastReadOffset);
                skipped = true;
            }

            if (skipped) {
                return receiveLockless(message);
            }
            
            if (messageLength > remainingSpace) {
                asm("hlt");
            }

            memcpy(message, buffer + lastReadOffset, messageLength);
            lastReadOffset += messageLength;
        }

        unreadMessages--;
        markSkippableRange(messageStart, messageLength);
        return true;
    }

    #if 0
    bool Mailbox::receiveLockless(Message* message) {

        if (unreadMessages == 0) {
            //block
            //kprintf("[IPC] Read blocked\n");
            //asm("hlt");
            return false;
        }
        else {
            uint32_t messageLength {0};
            bool tookWeirdPath {false};

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
                tookWeirdPath = true;
                messageLength = temp.length;
            }
            else {
                messageLength = reinterpret_cast<Message*>(buffer + lastReadOffset)->length;
            }

            if (messageLength > MaximumMessageSize) {
                int pause = 0;
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
                asm("hlt");
            }

            unreadMessages--;
            return true;
        }
    }
    #endif


    uint32_t Mailbox::findEndReadOffset() {
        auto ptr = buffer + lastReadOffset;
        auto currentReadOffset = lastReadOffset;

        for (int i = 0; i < unreadMessages; i++) {
            auto length = *reinterpret_cast<uint32_t*>(ptr);
            auto remainingSpace = bufferSize - currentReadOffset;

            if (length > remainingSpace) {
                length -= remainingSpace;
                ptr = buffer + length;
                currentReadOffset = length;
            }
            else {
                currentReadOffset += length;
            }
        }

        return currentReadOffset;
    }

    void Mailbox::markSkippableRange(uint32_t offset, uint32_t length) {
        /*auto message = reinterpret_cast<Message*>(buffer + offset);
        message->length = length;
        message->senderTaskId = 0;*/
        if (length <= 8) {
            int pause = 0;
        }
        *reinterpret_cast<uint32_t*>(buffer + offset) = length;
        offset += 4;

        if (offset >= bufferSize) {
            offset = 0;
        }

        *reinterpret_cast<uint32_t*>(buffer + offset) = 0;
    }

    bool Mailbox::canSkipRead(uint32_t start, uint32_t length) {
        uint32_t sender {0};
        uint32_t remainingSpace {0};

        if (lastReadOffset >= lastWriteOffset) {
            remainingSpace = bufferSize - lastReadOffset;
        }
        else {
            remainingSpace = lastWriteOffset - lastReadOffset;
        }

        if (remainingSpace >= 8) {
            sender = *reinterpret_cast<uint32_t*>(buffer + lastReadOffset + 4);
        }
        else {
            sender = *reinterpret_cast<uint32_t*>(buffer); 
        }

        return sender == 0;
    }

    bool Mailbox::skipRead(Message* message, uint32_t length) {

        if (length == 0) {
            auto limit = lastReadOffset > lastWriteOffset ? bufferSize : lastWriteOffset;

            for (int i = lastReadOffset; i < limit; i++) {
                if (buffer[i] > 0) {
                    lastReadOffset = i;
                    return false;
                }
            }    
        }
        else {
            lastReadOffset += length;

            if (lastReadOffset >= bufferSize) {
                lastReadOffset -= bufferSize;
            }
        }

        return false;
        //return receiveLockless(message);
    }

    #endif

}