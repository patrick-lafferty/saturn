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
#include "display.h"
#include "messages.h"
#include "lib/window.h"
#include <services.h>
#include <system_calls.h>
#include <services/keyboard/messages.h>
#include <services/mouse/messages.h>
#include <algorithm>
#include "container.h"
#include "tile.h"

using namespace Kernel;

namespace Apollo {

    Display::Display(Bounds screenBounds)
        : screenBounds {screenBounds} {
        root = new Container;
        root->split = Split::Horizontal;
        activeContainer = root;
        root->bounds = screenBounds;
    }

    void Display::addTile(Tile tile, Size size, bool focusable) {
        activeContainer->addChild(tile, size, focusable);
    }

    bool Display::enableRendering(uint32_t taskId) {

        if (auto tile = root->findTile(taskId)) {
            tile.value()->canRender = true;

            CreateWindowSucceeded success;
            success.recipientId = taskId;
            send(IPC::RecipientType::TaskId, &success);

            return true;
        }

        return false;
    }

    void Display::injectKeypress(Keyboard::KeyPress& message) {

        if (activeContainer->activeTaskId > 0) {
            message.recipientId = activeContainer->activeTaskId;
            send(IPC::RecipientType::TaskId, &message);
        }
    }

    void Display::injectCharacterInput(Keyboard::CharacterInput& message) {

       if (activeContainer->activeTaskId > 0) {
            message.recipientId = activeContainer->activeTaskId;
            send(IPC::RecipientType::TaskId, &message);
        } 
    }

    void Display::composite(uint32_t volatile* frameBuffer, uint32_t taskId, Bounds dirty) {
        auto maybeTile = root->findTile(taskId);

        if (maybeTile) { 
            auto& tile = *maybeTile.value();
            renderTile(frameBuffer, tile, screenBounds.width, dirty);
            tile.canRender = true;
        }
        
    }

    void Display::renderAll(uint32_t volatile* frameBuffer) {
        root->composite(frameBuffer, screenBounds.width);
    }

    void Display::splitContainer(Split split) {
        auto container = new Container();
        container->split = split;
        activeContainer->addChild(container, {});
        activeContainer = container;
    }

    void Display::render() {
        root->dispatchRenderMessages();
    }

    void Display::focusPreviousTile() {
        auto container = activeContainer;

        while (container != nullptr) {
            if (!activeContainer->focusPreviousTile()) {
                container = activeContainer->parent;

                if (container != nullptr) {
                    activeContainer = container;
                }
            }
        }
    }

    void Display::focusNextTile() {
        activeContainer->focusNextTile();
    }

    void Display::changeSplitDirection(Split split) {
        activeContainer->split = split;
        activeContainer->layoutChildren();
    }

    uint32_t Display::getActiveTaskId() const {
        return activeContainer->activeTaskId;
    }

}
    