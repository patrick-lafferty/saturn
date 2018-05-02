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
#include "tile.h"

namespace Keyboard {
    struct KeyPress;
    struct CharacterInput;
}

namespace Apollo {

    struct Container;

    class Display {
    public:

        Display(Bounds screenBounds);

        void addTile(Tile tile, Size size = {}, bool focusable = true);
        bool enableRendering(uint32_t taskId);
        void injectKeypress(Keyboard::KeyPress& message);
        void injectCharacterInput(Keyboard::CharacterInput& message);
        void composite(uint32_t volatile* frameBuffer, uint32_t taskId, Bounds dirty);
        void renderAll(uint32_t volatile* frameBuffer);
        void splitContainer(Split split);
        void render();
        void focusPreviousTile();
        void focusNextTile();
        void changeSplitDirection(Split split);

        uint32_t getActiveTaskId() const;

    private:

        Bounds screenBounds;
        Container* root;
        Container* activeContainer;
    };
 
}