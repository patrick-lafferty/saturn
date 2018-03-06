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
        uint32_t x {0}, y {0};
        uint32_t width {800}, height {600};
        bool readyToRender {false};
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

        std::vector<WindowHandle> windows;
        std::list<WindowHandle> windowsWaitingToShare;
        uint32_t volatile* linearFrameBuffer;

        uint32_t screenWidth {800u};
        uint32_t screenHeight {600u};

        uint32_t capcomTaskId;
        uint32_t capcomWindowId {0};

        uint32_t activeWindow {0};
        uint32_t previousActiveWindow {0};
        bool hasFocus {false};
    };
}