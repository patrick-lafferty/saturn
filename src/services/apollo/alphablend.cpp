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
#include <stdint.h>

namespace Apollo {
  
    uint32_t blend(uint32_t source, uint32_t destination, uint8_t alpha) {
        auto sourceRed = (source >> 16) & 0xFF;
        auto sourceGreen = (source >> 8) & 0xFF;
        auto sourceBlue = source & 0xFF;

        auto destinationRed = (destination >> 16) & 0xFF;
        auto destinationGreen = (destination >> 8) & 0xFF;
        auto destinationBlue = destination & 0xFF;

        auto destinationAlpha = 255 - alpha;

        uint32_t red = ((sourceRed * alpha) + (destinationRed * destinationAlpha)) /  255;
        uint32_t green = ((sourceGreen * alpha) + (destinationGreen * destinationAlpha)) / 255;
        uint32_t blue = ((sourceBlue * alpha) + (destinationBlue * destinationAlpha)) / 255;

        auto result = (255 << 24) | (red << 16) | (green << 8) | blue;

        return result;
    }
}