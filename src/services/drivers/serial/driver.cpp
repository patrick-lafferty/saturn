/*
Copyright (c) 2018, Patrick Lafferty
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
#include "driver.h"
#include <stdint.h>
#include <services.h>
#include <services/virtualFileSystem/messages.h>
#include <system_calls.h>

namespace Serial {

    void registerDriver() {
        Kernel::RegisterDriver request;
        request.type = Kernel::DriverType::Serial;
        send(IPC::RecipientType::ServiceRegistryMailbox, &request);

        IPC::MaximumMessageBuffer buffer;
        filteredReceive(&buffer, IPC::MessageNamespace::ServiceRegistry, 
            static_cast<uint32_t>(Kernel::MessageId::RegisterDriverResult));

        return;
    }

    void writeByte(int portOffset, uint8_t data) {
        uint16_t port {0x3f8};
        port += portOffset;

        asm("outb %0, %1"
            : //no output
            : "a"(data), "Nd" (port));
    }

    bool canTransmit() {
        uint8_t result;
        uint16_t port {0x3f8 + 5};

        asm("inb %1, %0"
            : "=a" (result)
            : "Nd" (port));

        return result & 0x20;
    }

    void transmit(uint8_t data) {
        while (!canTransmit()) {}

        writeByte(0, data);
    }

    void setupSerialPort() {
        //disable interrupts
        writeByte(1, 0);

        /*
        Configure baud rate
        */
        writeByte(3, 0x80); 
        writeByte(0, 0x03); 
        writeByte(1, 0x00); 

        //configure parity
        writeByte(3, 0x03); 

        //configure fifo
        writeByte(2, 0xC7); 
    }

    void mount() {
        VirtualFileSystem::MountRequest request;
        const char* path = "/serial";
        memcpy(request.path, path, strlen(path) + 1);
        request.serviceType = Kernel::ServiceType::VFS;
        request.cacheable = false;
        request.writeable = true;

        send(IPC::RecipientType::ServiceName, &request);
    }

    void service() {
        registerDriver();
        setupSerialPort();
        mount();

        while (true) {
            IPC::MaximumMessageBuffer buffer;
            receive(&buffer);

            switch (buffer.messageNamespace) {
                case IPC::MessageNamespace::VFS: {
                    
                    switch (static_cast<VirtualFileSystem::MessageId>(buffer.messageId)) {
                        case VirtualFileSystem::MessageId::OpenRequest: {
                            auto request = IPC::extractMessage<VirtualFileSystem::OpenRequest>(buffer);
                            
                            VirtualFileSystem::OpenResult result;
                            result.requestId = request.requestId;
                            result.serviceType = Kernel::ServiceType::VFS;
                            result.success = true;
                            result.fileDescriptor = 0;
                            send(IPC::RecipientType::ServiceName, &result);

                            break;
                        }
                        case VirtualFileSystem::MessageId::WriteRequest: {
                            auto request = IPC::extractMessage<VirtualFileSystem::WriteRequest>(buffer);

                            VirtualFileSystem::WriteResult result;
                            result.requestId = request.requestId;
                            result.success = true;
                            result.recipientId = request.senderTaskId;
                            result.expectReadResult = false;
                            send(IPC::RecipientType::TaskId, &result);

                            for (auto i = 0u; i < request.writeLength; i++) {
                                transmit(request.buffer[i]);
                            }

                            transmit('\n');

                            break;
                        }
                        default: {
                            break;
                        }
                    }

                    break;
                }
                default:
                    break;
            }
        }
    }
}