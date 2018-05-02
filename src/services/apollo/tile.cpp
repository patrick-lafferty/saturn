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
#include "tile.h"
#include <algorithm>
#include "lib/window.h"

namespace Apollo {

    void renderTile(uint32_t volatile* frameBuffer, Tile& tile, uint32_t displayWidth, Bounds dirty) {
        auto endY = dirty.y + std::min(tile.bounds.height, dirty.height);
        auto endX = dirty.x + std::min(tile.bounds.width, dirty.width);
        auto windowBuffer = tile.handle.buffer->buffer;

        for (auto y = dirty.y; y < endY; y++) {
            auto windowOffset = y * tile.stride;
            auto screenOffset = tile.bounds.x + (tile.bounds.y + y) * displayWidth;

            void* dest = const_cast<uint32_t*>(frameBuffer) + screenOffset;
            void* source = windowBuffer + windowOffset;

            memcpy(dest, source,
                sizeof(uint32_t) * (endX - dirty.x)); 
        }
    }
 
}
    