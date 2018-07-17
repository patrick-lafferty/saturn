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

        if (freeMessages > 0) {
            
            for (auto i = 0; i < MaxMessages; i++) {
                if (messages[i].buffer.length == 0) {
                    memcpy(&messages[i].buffer.buffer, message, message->length);

                    if (firstMessage == nullptr) {
                        firstMessage = &messages[i];
                        lastMessage = firstMessage;
                    }
                    else {
                        lastMessage->next = &messages[i];
                        lastMessage = &messages[i];
                    }

                    break;
                }
            }

            freeMessages--;
            unreadMessages++;
        }
        else {
            asm("cli");
            asm("hlt");
        }
    }

    bool Mailbox::receive(Message* message) {
        Kernel::SpinLock lock {&bufferLock};

        if (unreadMessages == 0) {
            return false;
        }

        memcpy(message, firstMessage->buffer.buffer, firstMessage->buffer.length);

        unreadMessages--;
        freeMessages++;
        firstMessage->buffer.length = 0;

        auto next = firstMessage->next;
        firstMessage->next = nullptr;
        firstMessage = next;

        if (firstMessage == nullptr) {
            lastMessage = nullptr;
        }

        return true;

        /*
        for (auto i = 0; i < MaxMessages; i++) {
            if (messages[i].buffer.length > 0) {
                memcpy(message, &messages[i].buffer, messages[i].buffer.length);
                unreadMessages--;
                freeMessages++;
                messages[i].buffer.length = 0;
                return true;
            }
        }*/

        return false;
    }

    bool Mailbox::filteredReceive(Message* message, IPC::MessageNamespace filter, uint32_t messageId) {
        Kernel::SpinLock lock {&bufferLock};

        if (unreadMessages == 0) {
            return false;
        }


        auto iterator = firstMessage;
        StoredMessage* previous {nullptr};

        while (iterator != nullptr) {

            if (iterator->buffer.messageNamespace == filter
                && iterator->buffer.messageId == messageId) {
                
                memcpy(message, iterator->buffer.buffer, iterator->buffer.length);

                unreadMessages--;
                freeMessages++;
                iterator->buffer.length = 0;

                if (previous != nullptr) {
                    previous->next = iterator->next;

                    if (iterator->next == nullptr) {
                        lastMessage = previous;
                    }
                }
                else {
                    firstMessage = iterator->next;

                    if (firstMessage == nullptr) {
                        lastMessage = nullptr;
                    }
                }

                return true;
            }

            previous = iterator;
            iterator = iterator->next;
        }

        /*
        for (auto i = 0; i < MaxMessages; i++) {
            if (messages[i].buffer.length > 0
                && messages[i].buffer.messageNamespace == filter
                && messages[i].buffer.messageId == messageId) {
                memcpy(message, &messages[i].buffer, messages[i].buffer.length);
                unreadMessages--;
                freeMessages++;
                messages[i].buffer.length = 0;
                return true;
            }
        }
        */

        return false;
    }

}