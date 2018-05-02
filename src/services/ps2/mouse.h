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
#pragma once

#include <stdint.h>
#include <ipc.h>

namespace PS2 {
    
    void initializeMouse();

    enum class MouseCommand {
        SetResolution = 0xE8,
        StatusRequest = 0xE9,
        RequestSinglePacket = 0xEB,
        GetMouseId = 0xF2,
        SetSampleRate = 0xF3,
        EnablePacketStreaming = 0xF4,
        DisablePacketStreaming = 0xF5,
        SetDefaults = 0xF6,
        Resend = 0xFe,
        Reset = 0xFF
    };

    enum class NextByte {
        Header,
        XMovement,
        YMovement,
        Optional
    };

    struct MouseState {
        uint8_t header {0};
        uint8_t xMovement {0};
        uint8_t yMovement {0};
        uint8_t optional {0};

        bool expectsOptional {false};
        NextByte next {NextByte::Header};
    };

    void transitionMouseState(MouseState& mouse, uint8_t data);
}