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

namespace Window {

    struct alignas(0x1000) WindowBuffer {
        uint32_t buffer[800 * 600];
    };

    struct DirtyArea {
        uint32_t x, y;
        uint32_t width, height;
    };

    class Window {
    public:

        Window(WindowBuffer* buffer, uint32_t width, uint32_t height); 

        uint32_t* getFramebuffer();
        uint32_t getBackgroundColour();
        void setBackgroundColour(uint32_t colour);
        uint32_t getWidth();
        void blitBackBuffer();
        void markAreaDirty(uint32_t x, uint32_t y, uint32_t width, uint32_t height);

    private:

        WindowBuffer* buffer;
        WindowBuffer* backBuffer;
        uint32_t backgroundColour;
        uint32_t width, height;
        bool dirty;
        DirtyArea dirtyArea;
    };

    Window* createWindow(uint32_t width, uint32_t height);
}