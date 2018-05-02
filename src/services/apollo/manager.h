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
#include <string>

namespace Kernel {
    struct ShareMemoryResult;
}

namespace Keyboard {
    struct KeyPress;
    struct CharacterInput;
}

namespace Mouse {
    struct MouseMove;
    struct ButtonPress;
    struct Scroll;
}

namespace Saturn::Log {
    class Logger;
}

namespace Apollo {

    int main();

    struct WindowHandle {
        struct WindowBuffer* buffer;
        uint32_t taskId;
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
        struct Container* parent {nullptr};
        bool canRender {false};
        uint32_t stride {800};
    };

    enum class Split;

    enum class Unit {
        Proportional,
        Fixed
    };

    struct Size {
        Unit unit {Unit::Proportional};
        int desiredSpace {1};
        int actualSpace {0};
    };

    struct ContainerChild {
        Size size;
        bool focusable {true};
        std::variant<Tile, Container*> child;
    };

    struct Container {
        Bounds bounds;
        std::vector<ContainerChild> children;
        Split split;
        Container* parent;
        uint32_t activeTaskId {0};

        void addChild(Tile tile, Size size, bool focusable);
        void addChild(Container* container, Size size);
        void layoutChildren();
        uint32_t getChildrenCount();
        std::optional<Tile*> findTile(uint32_t taskId);
        void composite(uint32_t volatile* frameBuffer, uint32_t displayWidth);
        void dispatchRenderMessages();
        bool focusPreviousTile();
        bool focusNextTile();
    };

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

    struct LayoutVisitor {
        void visit(Tile&);
        void visit(Container*);

        LayoutVisitor(Bounds b, Split s) 
            : bounds {b}, split {s}
            {}

        void updateBounds();

        Bounds bounds;
        Split split;
    };

    class Manager {
    public:

        Manager(uint32_t framebufferAddress);
        void messageLoop();

    private:

        uint32_t launch(const char* path);
        void handleCreateWindow(const struct CreateWindow& message);
        void handleUpdate(const struct Update& message);
        void handleMove(const struct Move& message);
        void handleReadyToRender(const struct ReadyToRender& message);
        void handleSplitContainer(const struct SplitContainer& message);
        void handleLaunchProgram(const struct LaunchProgram& message);
        void handleHideOverlay();
        void handleChangeSplitDirection(const struct ChangeSplitDirection& message);

        void handleShareMemoryResult(const Kernel::ShareMemoryResult& message);

        void handleKeyPress(Keyboard::KeyPress& message);

        void handleMouseMove(Mouse::MouseMove& message);
        void handleMouseButton(Mouse::ButtonPress& message);
        void handleMouseScroll(Mouse::Scroll& message);

        uint32_t volatile* linearFrameBuffer;

        uint32_t screenWidth {800u};
        uint32_t screenHeight {600u};

        uint32_t capcomTaskId;
        uint32_t taskbarTaskId {0};

        bool hasFocus {false};

        std::vector<Display> displays;
        uint32_t currentDisplay {0};
        uint32_t previousDisplay {1};
        bool showCapcom {false};

        struct PendingTaskbarApp {
            std::string name;
            uint32_t taskId;
        };

        std::vector<PendingTaskbarApp> pendingTaskbarNames;

        int mouseX {400}, mouseY {300};

        Saturn::Log::Logger* logger;
    };
}