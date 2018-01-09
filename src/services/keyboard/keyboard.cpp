/*
Copyright (c) 2017, Patrick Lafferty
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
#include "keyboard.h"
#include <services.h>
#include <system_calls.h>
#include <services/terminal/terminal.h>
#include <string.h>
#include <stdio.h>

using namespace Kernel;
using namespace PS2;

namespace Keyboard {

    struct KeymapEntry {
        uint8_t normal;
        uint8_t shift;
    };

    KeymapEntry en_US[128];

    void loadKeymap() {
        en_US[0x1C] = {'a', 'A'};
        en_US[0x32] = {'b', 'B'};
        en_US[0x21] = {'c', 'C'};
        en_US[0x23] = {'d', 'D'};
        en_US[0x24] = {'e', 'E'};
        en_US[0x2B] = {'f', 'F'};
        en_US[0x34] = {'g', 'G'};
        en_US[0x33] = {'h', 'H'};
        en_US[0x43] = {'i', 'I'};
        en_US[0x3B] = {'j', 'J'};
        en_US[0x42] = {'k', 'K'};
        en_US[0x4B] = {'l', 'L'};
        en_US[0x3A] = {'m', 'M'};
        en_US[0x31] = {'n', 'N'};
        en_US[0x44] = {'o', 'O'};
        en_US[0x4D] = {'p', 'P'};
        en_US[0x15] = {'q', 'Q'};
        en_US[0x2D] = {'r', 'R'};
        en_US[0x1B] = {'s', 'S'};
        en_US[0x2C] = {'t', 'T'};
        en_US[0x3C] = {'u', 'U'};
        en_US[0x2A] = {'v', 'V'};
        en_US[0x1D] = {'w', 'W'};
        en_US[0x22] = {'x', 'X'};
        en_US[0x35] = {'y', 'Y'};
        en_US[0x1A] = {'z', 'Z'};

        en_US[0x45] = {'0', ')'};
        en_US[0x16] = {'1', '!'};
        en_US[0x1E] = {'2', '@'};
        en_US[0x26] = {'3', '#'};
        en_US[0x25] = {'4', '$'};
        en_US[0x2E] = {'5', '%'};
        en_US[0x36] = {'6', '^'};
        en_US[0x3D] = {'7', '&'};
        en_US[0x3E] = {'8', '*'};
        en_US[0x46] = {'9', '('};

        //keypad
        en_US[0x70] = {'0', 0};
        en_US[0x69] = {'1', 0};
        en_US[0x72] = {16, 6}; //down arrow
        en_US[0x7A] = {'3', 0};
        en_US[0x6B] = {14, 14}; //left arrow (should be keypad 4)
        en_US[0x73] = {'5', 0};
        en_US[0x74] = {17, 17}; //right arrow
        en_US[0x6C] = {'7', 0};
        en_US[0x75] = {15, 15}; //up arrow (should be keypad 8 but qemu has a bug)
        en_US[0x7D] = {'9', 0};
        en_US[0x71] = {'.', 0};
        en_US[0x79] = {'+', 0};
        en_US[0x7B] = {'-', 0};
        en_US[0x7C] = {'*', 0};

        en_US[0x0E] = {'`', '~'};
        en_US[0x29] = {' ', 0};
        en_US[0x41] = {',', '<'};
        en_US[0x49] = {'.', '>'};
        en_US[0x4A] = {'/', '?'};
        en_US[0x4C] = {';', ':'};
        en_US[0x4E] = {'-', '_'};
        en_US[0x52] = {'\'', '"'};
        en_US[0x54] = {'[', '{'};
        en_US[0x5B] = {']', '}'};
        en_US[0x55] = {'=', '+'};
        en_US[0x5D] = {'\\', '|'};

        en_US[0x5A] = {13, 13};
        en_US[0x66] = {8, 8};
    }

    struct ModifierStatus {
        bool leftShiftPressed {false};
        bool rightShiftPressed {false};
        bool capsLockPressed {false};
    };

    uint8_t translateKeyEvent(PhysicalKey key, ModifierStatus& status) {
        /*TODO: keymaps should be stored in files, but since
        we don't have filesystem support yet, just hardcode
        en_US in the above array
        */
        auto& entry = en_US[static_cast<uint32_t>(key)];

        if (status.leftShiftPressed || status.rightShiftPressed || status.capsLockPressed) {
            return entry.shift;
        }
        else {
            return entry.normal;
        }
    }

    void echo(uint8_t key) {
        Terminal::PrintMessage message {};
        message.serviceType = Kernel::ServiceType::Terminal;
        char s[] = {(char)key, '\0'};
        message.stringLength = strlen(s);
        memcpy(message.buffer, s, message.stringLength);
        send(IPC::RecipientType::ServiceName, &message);
    }

    void sendKeyToTerminal(uint8_t key) {
        Terminal::KeyPress message {};
        message.serviceType = Kernel::ServiceType::Terminal;
        message.key = key;
        send(IPC::RecipientType::ServiceName, &message);
    }
    
    void messageLoop() {

        ModifierStatus modifiers {};

        while (true) {
            IPC::MaximumMessageBuffer buffer;
            receive(&buffer);

            if (buffer.messageNamespace == IPC::MessageNamespace::Keyboard
                && buffer.messageId == static_cast<uint32_t>(MessageId::KeyEvent)) {
                auto event = IPC::extractMessage<KeyEvent>(buffer);

                switch (event.key) {
                    case PhysicalKey::LeftShift: {
                        modifiers.leftShiftPressed = event.status == KeyStatus::Pressed;
                        break;
                    }
                    case PhysicalKey::RightShift: {
                        modifiers.rightShiftPressed = event.status == KeyStatus::Pressed;
                        break;
                    }
                    case PhysicalKey::CapsLock: {
                        if (event.status == KeyStatus::Pressed) {
                            modifiers.capsLockPressed = !modifiers.capsLockPressed;
                        }
                        break;
                    }
                    default: {
                        
                        if (event.status == KeyStatus::Pressed) {
                            auto key = translateKeyEvent(event.key, modifiers);
                            sendKeyToTerminal(key);
                            //echo(key);
                        }
                        break;
                    }
                }
            }
        }
    }

    void registerService() {
        RegisterService registerRequest {};
        registerRequest.type = ServiceType::Keyboard;

        IPC::MaximumMessageBuffer buffer;

        send(IPC::RecipientType::ServiceRegistryMailbox, &registerRequest);
        receive(&buffer);
    }

    void service() {
        registerService();        
        loadKeymap();
        messageLoop();
    }
}