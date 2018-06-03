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
#include "mouse.h"
#include "ps2.h"
#include <services.h>
#include <system_calls.h>
#include <services/mouse/messages.h>
#include <services/virtualFileSystem/vostok.h>

namespace PS2 {

    bool writeMouseCommand(MouseCommand command) {
        writeCommand(static_cast<uint8_t>(command), Port::Second);

        return waitForAck();
    }

    bool writeMouseData(uint8_t data) {
        writeCommand(data, Port::Second);

        return waitForAck();
    }

    bool setSampleRate(int rate) {

        return writeMouseCommand(MouseCommand::SetSampleRate)
            && writeMouseData(rate);
    }

    bool setResolution(int resolution) {

        return writeMouseCommand(MouseCommand::SetResolution)
            && writeMouseData(resolution);
    }

    bool enableIntellimouse() {
        if (!setSampleRate(200)
            || !setSampleRate(100)
            || !setSampleRate(80)) {
            return false;
        }

        writeCommand(static_cast<uint8_t>(MouseCommand::GetMouseId), Port::Second);

        if (!waitForAck()) {
            return false;
        }

        auto id = readByte();

        return id == static_cast<uint8_t>(DeviceType::ScrollMouse);
    }

    void initializeMouse() {
        if (enableIntellimouse()) {
            auto createResult = createSynchronous("/system/hardware/mouse");

            if (!createResult.success) {
                return;
            }

            auto openResult = openSynchronous("/system/hardware/mouse/hasScrollWheel");

            if (!openResult.success) {
                return;
            }

            auto readResult = readSynchronous(openResult.fileDescriptor, 0);

            Vostok::ArgBuffer args{readResult.buffer, sizeof(readResult.buffer)};
            args.readType();

            args.writeValueWithType(true, Vostok::ArgTypes::Bool);
            writeSynchronous(openResult.fileDescriptor, readResult.buffer, sizeof(readResult.buffer));

            close(openResult.fileDescriptor);
            receiveAndIgnore();
        }

        if (!setResolution(0)) {
            asm("hlt");
        }

        if (!setSampleRate(40)) {
            asm("hlt");
        }

        if (!writeMouseCommand(MouseCommand::EnablePacketStreaming)) {
            asm("hlt");
        }
    }

    void transitionMouseState(MouseState& state, uint8_t data) {
        bool canSend {false};

        switch (state.next) {
            case NextByte::Header: {
                
                if (data & 0x80 || data & 0x40) {
                    return;
                }

                state.header = data;
                state.next = NextByte::XMovement;
                break;
            }
            case NextByte::XMovement: {
                state.xMovement = data;
                state.next = NextByte::YMovement;
                break;
            }
            case NextByte::YMovement: {
                state.yMovement = data;

                if (state.expectsOptional) {
                    state.next = NextByte::Optional;
                }
                else {
                    canSend = true;
                }

                break;
            }
            case NextByte::Optional: {
                state.optional = data;
                canSend = true;

                break;
            }
        }

        if (canSend) {
            state.next = NextByte::Header;

            Mouse::MouseEvent event {};
            event.serviceType = Kernel::ServiceType::Mouse;
            event.header = state.header;
            event.xMovement = state.xMovement;
            event.yMovement = state.yMovement;
            event.optional = state.optional;

            send(IPC::RecipientType::ServiceName, &event);
        }
    }
}