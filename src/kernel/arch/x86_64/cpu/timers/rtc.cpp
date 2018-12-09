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
#include "rtc.h"

namespace RTC {

    void selectIndex(uint8_t index) {
        uint16_t rtcIndexPort {0x70};

        asm("out %0, %1"
            : //no output
            : "a" (index), "Nd" (rtcIndexPort));
    }

    void writePort(uint8_t value) {
        uint16_t rtcIOPort {0x71};

        asm("out %0, %1"
            : //no output
            : "a" (value), "Nd" (rtcIOPort));
    }

    uint8_t readPort() {
        uint8_t result;
        uint16_t rtcIOPort {0x71};

        asm("inb %1, %0"
            : "=a" (result)
            : "Nd" (rtcIOPort));

        return result;
    }

    void enable(uint8_t rate) {
        uint8_t statusRegisterB {0};

        selectIndex(0x8B);
        statusRegisterB = readPort();
        selectIndex(0x8B);

        statusRegisterB |= 0x40;

        writePort(statusRegisterB);

        selectIndex(0x8A);
        auto statusRegisterA = readPort();
        selectIndex(0x8A);
        /*
        0xD corresponds to a rate of 13
        frequency is calculated as:
        32768 >> (rate - 1)

        so this gives a rate of 8Hz
        */
        writePort((statusRegisterA & 0xF0) | rate);
    }

    void disable() {
        uint8_t statusRegisterB {0};

        selectIndex(0x8B);
        statusRegisterB = readPort();
        selectIndex(0x8B);
        statusRegisterB &= ~0x40;
        writePort(statusRegisterB);
    }

    void reset() {
        selectIndex(0xC);
        readPort();
    }
}