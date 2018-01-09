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
#include "permissions.h"
#include <stdio.h>

namespace Kernel {
    
    void grantIOPort8(uint16_t port, uint8_t volatile* iopb) {
        auto index = port / 8;
        auto bit = port % 8;

        auto byte = iopb + index;
        *byte &= ~(1 << bit);
    }
    
    void grantIOPort16(uint16_t port, uint8_t volatile* iopb) {
        grantIOPort8(port, iopb);
        grantIOPort8(port + 1, iopb);
    }

    void blockIOPort16(uint16_t port, uint8_t volatile* iopb) {
        auto index = port / 8;
        auto bit = port % 8;

        auto byte = iopb + index;
        *byte |= (1 << bit);

        if (bit == 7) {
            byte++;
            *byte |= 1;
        }
        else {
            *byte |= (1 << (bit + 1));
        }
    }

    void grantIOPortRange(uint16_t portStart, uint16_t portEnd, uint8_t volatile* iopb) {

        for (auto port = portStart; port != portEnd + 1; port++) {
            grantIOPort8(port, iopb);
        }
    }
}