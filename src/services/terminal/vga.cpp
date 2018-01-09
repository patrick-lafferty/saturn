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
#include "vga.h"
#include <services.h>
#include "terminal.h"
#include <system_calls.h>

using namespace Kernel;

namespace VGA {

    void messageLoop(uint32_t address) {
        uint16_t* vgaBuffer = reinterpret_cast<uint16_t*>(address);
        memset(vgaBuffer, 0, 4000);

        while (true) {
            IPC::MaximumMessageBuffer buffer;
            receive(&buffer);

            switch(buffer.messageNamespace) {
                case IPC::MessageNamespace::VGA: {

                    switch(static_cast<MessageId>(buffer.messageId)) {
                        case MessageId::Blit: {
                            auto message = IPC::extractMessage<BlitMessage>(buffer);
                            memcpy(vgaBuffer + message.index, message.buffer, sizeof(uint16_t) * message.count);

                            break;
                        }
                        case MessageId::Scroll: {
                            auto message = IPC::extractMessage<ScrollScreen>(buffer);
                            auto byteCount = sizeof(uint16_t) * Width * (Height - message.linesToScroll);
                            memcpy(vgaBuffer, vgaBuffer + Width * message.linesToScroll, byteCount);

                            auto row = Height - 1;

                            for (uint32_t x = 0; x < Width; x++) {
                                auto index = row * Width + x;
                                vgaBuffer[index] = 0;
                            }

                            break;
                        }
                    }

                    break;
                }
            }
        }
    }

    uint32_t registerService() {
        RegisterService registerRequest {};
        registerRequest.type = ServiceType::VGA;

        IPC::MaximumMessageBuffer buffer;

        send(IPC::RecipientType::ServiceRegistryMailbox, &registerRequest);
        receive(&buffer);

        switch(static_cast<IPC::MessageNamespace>(buffer.messageNamespace)) {
            case IPC::MessageNamespace::ServiceRegistry: {

                switch(static_cast<Kernel::MessageId>(buffer.messageId)) {
                    case Kernel::MessageId::RegisterServiceDenied: {
                        //TODO: stub
                        break;
                    }
                    case Kernel::MessageId::VGAServiceMeta: {
                        NotifyServiceReady ready;
                        send(IPC::RecipientType::ServiceRegistryMailbox, &ready);

                        auto vgaMeta = IPC::extractMessage<VGAServiceMeta>(buffer);

                        return vgaMeta.vgaAddress;
                        break;
                    }
                }

                break;
            }
        }

        return 0;
    }

    void service() {
        auto vgaAddress = registerService();
        messageLoop(vgaAddress);
    }

    uint8_t getColour(Colours foreground, Colours background) {
        return static_cast<uint8_t>(foreground) | static_cast<uint8_t>(background) << 4;
    }

    void setForegroundColour(uint8_t& colour, uint8_t foregroundColour) {
        colour = foregroundColour | (colour & 0xF0);
    }

    void setBackgroundColour(uint8_t& colour, uint8_t backgroundColour) {
        colour = (backgroundColour << 4) | (colour & 0x0F);
    }

    uint16_t prepareCharacter(uint8_t character, uint8_t colour) {
        return static_cast<uint16_t>(character) | static_cast<uint16_t>(colour) << 8;
    }

    void disableCursor() {
        uint8_t cursorStartRegister {0x0A};
        uint8_t disableCursor {1 << 5};

        uint16_t commandPort {0x3D4};
        uint16_t dataPort {0x3D5};

        asm("outb %0, %1"
            : //no output
            : "a" (cursorStartRegister), "Nd" (commandPort));

        asm("outb %0, %1"
            : //no output
            : "a" (disableCursor), "Nd" (dataPort));
    }
}