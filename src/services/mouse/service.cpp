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
#include "service.h"
#include "messages.h"
#include <services.h>
#include <system_calls.h>

using namespace Kernel;
using namespace PS2;

namespace Mouse {

    struct MouseState {
        int x {0};
        int y {0};

        bool leftPressed {false};
        bool middlePressed {false};
        bool rightPressed {false};
    };

    void sendButtonEvent(Button button, ButtonState state) {
        ButtonPress press;
        press.serviceType = ServiceType::WindowManager;
        press.button = button;
        press.state = state;

        send(IPC::RecipientType::ServiceName, &press);
    }

    void sendScrollEvent(ScrollDirection direction, ScrollMagnitude magnitude) {
        Scroll scroll;
        scroll.serviceType = ServiceType::WindowManager;
        scroll.direction = direction;
        scroll.magnitude = magnitude;

        send(IPC::RecipientType::ServiceName, &scroll);
    }

    enum class ScrollValues {
        VerticalUp = 0x1,
        VerticalDown = 0xF,
        HorizontalLeft = 0xE,
        HorizontalRight = 0x2
    };
    
    void messageLoop() {

        MouseState state;
        const int leftButtonValue = 1;
        const int middleButtonValue = 4;
        const int rightButtonValue = 2;

        while (true) {
            IPC::MaximumMessageBuffer buffer;
            receive(&buffer);

            switch(buffer.messageNamespace) {
                case IPC::MessageNamespace::Mouse: {
                    switch (static_cast<MessageId>(buffer.messageId)) {
                        case MessageId::MouseEvent: {

                            auto event = IPC::extractMessage<MouseEvent>(buffer);

                            if (event.xMovement != 0 || event.yMovement != 0) {
                                MouseMove move;

                                move.serviceType = ServiceType::WindowManager;
                                move.deltaX = event.xMovement;
                                move.deltaY = event.yMovement;

                                if (event.header & 0x20) {
                                    move.deltaY = event.yMovement | 0xFFFFFF00;
                                }

                                if (event.header & 0x10) {
                                    move.deltaX = event.xMovement | 0xFFFFFF00;
                                }

                                send(IPC::RecipientType::ServiceName, &move);
                            }

                            if (event.header & leftButtonValue) {
                                if (!state.leftPressed) {
                                    sendButtonEvent(Button::Left, ButtonState::Pressed);
                                    state.leftPressed = true;
                                }
                            }
                            else if (state.leftPressed) {
                                sendButtonEvent(Button::Left, ButtonState::Released);
                                state.leftPressed = false;
                            }

                            if (event.header & rightButtonValue) {
                                if (!state.rightPressed) {
                                    sendButtonEvent(Button::Right, ButtonState::Pressed);
                                    state.rightPressed = true;
                                }
                            }
                            else if (state.rightPressed) {
                                sendButtonEvent(Button::Right, ButtonState::Released);
                                state.rightPressed = false;
                            }

                            if (event.header & middleButtonValue) {
                                if (!state.middlePressed) {
                                    sendButtonEvent(Button::Middle, ButtonState::Pressed);
                                    state.middlePressed = true;
                                }
                            }
                            else if (state.middlePressed) {
                                sendButtonEvent(Button::Middle, ButtonState::Released);
                                state.middlePressed = false;
                            }

                            switch (event.optional & 0xF) {
                                case static_cast<uint8_t>(ScrollValues::VerticalUp): {
                                    sendScrollEvent(ScrollDirection::Vertical, ScrollMagnitude::UpBy1);
                                    break;
                                }
                                case static_cast<uint8_t>(ScrollValues::VerticalDown): {
                                    sendScrollEvent(ScrollDirection::Vertical, ScrollMagnitude::DownBy1);
                                    break;
                                }
                                case static_cast<uint8_t>(ScrollValues::HorizontalLeft): {
                                    sendScrollEvent(ScrollDirection::Horizontal, ScrollMagnitude::UpBy1);
                                    break;
                                }
                                case static_cast<uint8_t>(ScrollValues::HorizontalRight): {
                                    sendScrollEvent(ScrollDirection::Horizontal, ScrollMagnitude::DownBy1);
                                    break;
                                }
                            }

                            break;
                        }
                        default: {
                            break;
                        }
                    }
                    break;
                }
                default: {
                    printf("[Mouse] Unhandled message namespace\n");
                }
            }    
        }
    }

    void registerService() {
        RegisterService registerRequest {};
        registerRequest.type = ServiceType::Mouse;

        IPC::MaximumMessageBuffer buffer;

        send(IPC::RecipientType::ServiceRegistryMailbox, &registerRequest);
        receive(&buffer);
    }

    void service() {
        registerService();        
        messageLoop();
    }
}