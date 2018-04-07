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

#include "renderer.h"
#include "text.h"
#include "window.h"
#include <algorithm>

namespace Apollo {

    Renderer::Renderer(Window* window, Text::Renderer* renderer) 
        : window {window}, textRenderer{renderer} {

    }

    void Renderer::drawRectangle(uint32_t colour, int x, int y, int width, int height) {
        auto windowWidth = static_cast<int>(window->getWidth());
        auto windowHeight = 600;

        if (x >= windowWidth || y >= windowHeight) {
            return;
        }

        auto clippedWidth = (x + width > windowWidth) ? (windowWidth - x) : width;
        auto clippedHeight = (y + height > windowHeight) ?  (windowHeight - y) : height;
        auto frameBuffer = window->getFramebuffer();

        for (auto row = 0; row < clippedHeight; row++) {
            std::fill_n(frameBuffer + x + ((y + row) * windowWidth), clippedWidth, colour);
        }

        window->markAreaDirty(x, y, clippedWidth, clippedHeight);
    }

    void Renderer::drawText(const Apollo::Text::TextLayout& layout, uint32_t x, uint32_t y, uint32_t backgroundColour) {
        textRenderer->drawText(layout, x, y, backgroundColour);
    }

    Text::Renderer* Renderer::getTextRenderer() {
        return textRenderer;
    }
}
