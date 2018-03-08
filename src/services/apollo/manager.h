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
#include <vector>
#include <list>
#include <variant>
#include <optional>

namespace Kernel {
    struct ShareMemoryResult;
}

namespace Keyboard {
    struct KeyPress;
    struct CharacterInput;
}

namespace Apollo {

    int main();

    struct WindowHandle {
        struct WindowBuffer* buffer;
        uint32_t taskId;
        /*uint32_t x {0}, y {0};
        uint32_t width {800}, height {600};
        bool readyToRender {false};*/
    };

    /*
    A Tile is a rectangular area where content is rendered.
    It can either be a Window, or a Container of Windows.
    */
    struct Bounds {
        uint32_t x {0}, y {0};
        uint32_t width {800}, height {600};
    };

    struct Tile {
        Bounds bounds;
        WindowHandle handle;
        //std::variant<WindowHandle, struct Container*> content;
        struct Container* parent {nullptr};
        bool canRender {false};
    };

    struct Overlay {
        Tile tile;
        bool visible {false};
    };

    enum class Split {
        Horizontal,
        Vertical
    };

    struct Container {
        Bounds bounds;
        std::vector<std::variant<Tile, Container*>> children;
        Split split {Split::Horizontal};
        Container* parent;
        uint32_t activeTaskId {0};

        void addChild(Tile tile);
        void addChild(Container* container);
        void layoutChildren();
        uint32_t getChildrenCount();
        std::optional<Tile*> findTile(uint32_t taskId);
        void render(uint32_t volatile* frameBuffer, uint32_t displayWidth);
    };

    class Display {
    public:

        Display(Bounds screenBounds);

        void addTile(Tile tile);
        bool enableRendering(uint32_t taskId);
        void injectKeypress(Keyboard::KeyPress& message);
        void injectCharacterInput(Keyboard::CharacterInput& message);
        void composite(uint32_t volatile* frameBuffer, uint32_t taskId, Bounds dirty);
        void renderAll(uint32_t volatile* frameBuffer);

    private:

        Bounds screenBounds;
        Container* root;
        Container* activeContainer;
    };

    struct RenderVisitor {
        void operator()(WindowHandle) {}
        void operator()(Container*) {}
    };

    struct LayoutVisitor {
        void operator()(Tile);
        void operator()(Container*);

        LayoutVisitor(Bounds& b, Split s) 
            : bounds {b}, split {s}
            {}

        void updateBounds();

        Bounds& bounds;
        Split split;
    };

    class Manager {
    public:

        Manager(uint32_t framebufferAddress);
        void messageLoop();

    private:

        void handleCreateWindow(const struct CreateWindow& message);
        void handleUpdate(const struct Update& message);
        void handleMove(const struct Move& message);
        void handleReadyToRender(const struct ReadyToRender& message);

        void handleShareMemoryResult(const Kernel::ShareMemoryResult& message);

        void handleKeyPress(Keyboard::KeyPress& message);

        //std::vector<WindowHandle> windows;
        std::list<WindowHandle> windowsWaitingToShare;
        std::list<Tile> tilesWaitingToShare;
        uint32_t volatile* linearFrameBuffer;

        uint32_t screenWidth {800u};
        uint32_t screenHeight {600u};

        uint32_t capcomTaskId;
        uint32_t capcomWindowId {0};

        uint32_t activeWindow {0};
        uint32_t previousActiveWindow {0};
        bool hasFocus {false};

        std::vector<Display> displays;
        uint32_t currentDisplay {1};
        uint32_t previousDisplay {1};
        bool showCapcom {false};

        //Overlay capcom;
    };
}