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
#include "container.h"
#include "display.h"

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