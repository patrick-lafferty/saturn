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
#include "print.h"

void printInteger(uint32_t i, int line, int column, bool isNegative, int base, bool upper) {
    char buffer[8 * sizeof(int) - 1];
    char hexDigits[] = {'0', '1', '2', '3', '4', '5', '6', '7', '8', '9',
        'a', 'b', 'c', 'd', 'e', 'f'};
    int digits {0};

    do {
        buffer[digits] = hexDigits[i % base];

        if (upper && buffer[digits] >= 'a') {
            buffer[digits] -= 32;
        }

        digits++;
        i /= base;
    } while (i > 0);

    if (isNegative) {
        buffer[digits++] = '-';
    }

    digits--;

    auto position = line * 80 + column;
    auto screen = reinterpret_cast<uint16_t*>(0xb8000) + position;

    while (digits >= 0) {
        *screen++ = buffer[digits] | (0xF << 8);
        digits--;
    }
}

void printString(const char* message, int line, int column) {

    auto position = line * 80 + column;
    auto buffer = reinterpret_cast<uint16_t*>(0xb8000) + position;

    while(message && *message != '\0') {
        *buffer++ = *message++ | (0xF << 8);
    }
}