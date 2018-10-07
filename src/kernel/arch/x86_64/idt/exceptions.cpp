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
#include "exceptions.h"
#include "descriptor.h"

namespace IDT {

    void loadExceptions() {
        idt[0] = encodeEntry(reinterpret_cast<uintptr_t>(&exception0), 0x08);
        idt[1] = encodeEntry(reinterpret_cast<uintptr_t>(&exception1), 0x08);
        idt[2] = encodeEntry(reinterpret_cast<uintptr_t>(&exception2), 0x08);
        idt[3] = encodeEntry(reinterpret_cast<uintptr_t>(&exception3), 0x08);
        idt[4] = encodeEntry(reinterpret_cast<uintptr_t>(&exception4), 0x08);
        idt[5] = encodeEntry(reinterpret_cast<uintptr_t>(&exception5), 0x08);
        idt[6] = encodeEntry(reinterpret_cast<uintptr_t>(&exception6), 0x08);
        idt[7] = encodeEntry(reinterpret_cast<uintptr_t>(&exception7), 0x08);
        idt[8] = encodeEntry(reinterpret_cast<uintptr_t>(&exception8), 0x08);
        idt[9] = encodeEntry(reinterpret_cast<uintptr_t>(&exception9), 0x08);
        idt[10] = encodeEntry(reinterpret_cast<uintptr_t>(&exception10), 0x08);
        idt[11] = encodeEntry(reinterpret_cast<uintptr_t>(&exception11), 0x08);
        idt[12] = encodeEntry(reinterpret_cast<uintptr_t>(&exception12), 0x08);
        idt[13] = encodeEntry(reinterpret_cast<uintptr_t>(&exception13), 0x08);
        idt[14] = encodeEntry(reinterpret_cast<uintptr_t>(&exception14), 0x08);
        idt[15] = encodeEntry(reinterpret_cast<uintptr_t>(&exception15), 0x08);
        idt[16] = encodeEntry(reinterpret_cast<uintptr_t>(&exception16), 0x08);
        idt[17] = encodeEntry(reinterpret_cast<uintptr_t>(&exception17), 0x08);
        idt[18] = encodeEntry(reinterpret_cast<uintptr_t>(&exception18), 0x08);
        idt[19] = encodeEntry(reinterpret_cast<uintptr_t>(&exception19), 0x08);
        idt[20] = encodeEntry(reinterpret_cast<uintptr_t>(&exception20), 0x08);
    }
}

void printString2(const char* message, int line, int column) {

    auto position = line * 80 + column;
    auto buffer = reinterpret_cast<uint16_t*>(0xb8000) + position;

    while(message && *message != '\0') {
        *buffer++ = *message++ | (0xF << 8);
    }
}

void exceptionHandler(ExceptionFrame* frame) {
    char msg[] = {0, 0, 0};
    msg[0] = '0' + (frame->error / 10);
    msg[1] = '0' + (frame->error % 10);
    static int line = 1;
    printString2(msg, line++, 0);
}
