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

namespace IPC {

    void Mailbox::send(Message* message) {
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

    bool Mailbox::receive(Message* message) {
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

}