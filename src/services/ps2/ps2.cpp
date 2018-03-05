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
#include "ps2.h"
#include <services.h>
#include <system_calls.h>
#include <services/terminal/terminal.h>
#include <string.h>
#include <stdio.h>
#include <services/keyboard/messages.h>

using namespace Kernel;

namespace PS2 {

    void printDebugString(char* s) {
        Terminal::PrintMessage message {};
        message.serviceType = Kernel::ServiceType::Terminal;
        message.stringLength = strlen(s);
        memcpy(message.buffer, s, message.stringLength);
        send(IPC::RecipientType::ServiceName, &message);
    }

    enum class KeyboardStates {
        Start,
        E0,
        E0F0,
        F0,
    };

    void sendToKeyboard(PhysicalKey key, KeyStatus status) {
        Keyboard::KeyEvent event {};
        event.serviceType = ServiceType::Keyboard;
        event.key = key;
        event.status = status;
               
        send(IPC::RecipientType::ServiceName, &event);
    }

    KeyboardStates transitionKeyboardState(KeyboardStates initial, uint8_t data) {
        switch(initial) {
            case KeyboardStates::Start: {

                if (data < 0x7E) {
                    auto key = static_cast<PhysicalKey>(data);
                    sendToKeyboard(key, KeyStatus::Pressed);

                    return KeyboardStates::Start;
                }
                else if (data == 0xF0) {
                    return KeyboardStates::F0;
                }

                break;
            }
            case KeyboardStates::F0: {
                auto key = static_cast<PhysicalKey>(data);
                sendToKeyboard(key, KeyStatus::Released);

                return KeyboardStates::Start;
            }
            case KeyboardStates::E0:
            case KeyboardStates::E0F0: {
                //TODO: not implemented yet, just blow up
                asm("hlt");
            }
        }

        //should never get here
        return initial;
    }

    void messageLoop() {
        KeyboardStates keyboardState {KeyboardStates::Start};

        while (true) {
            IPC::MaximumMessageBuffer buffer;
            receive(&buffer);

            switch(buffer.messageNamespace) {
                case IPC::MessageNamespace::PS2: {

                    switch(static_cast<MessageId>(buffer.messageId)) {
                        case MessageId::KeyboardInput: {
                            auto input = IPC::extractMessage<KeyboardInput>(buffer);
                            keyboardState = transitionKeyboardState(keyboardState, input.data);

                            break;
                        }
                        case MessageId::MouseInput: {
                            break;
                        }
                    }
                    break;
                }
                default: 
                    break;
            }
        }
    }

    void service() {
        RegisterService registerRequest {};
        registerRequest.type = ServiceType::PS2;

        IPC::MaximumMessageBuffer buffer;

        send(IPC::RecipientType::ServiceRegistryMailbox, &registerRequest);
        receive(&buffer);

        if (buffer.messageNamespace == IPC::MessageNamespace::ServiceRegistry) {

            if (buffer.messageId == static_cast<uint32_t>(Kernel::MessageId::RegisterServiceDenied)) {
                //can't register ps2 device?
            }
            else if (buffer.messageId == static_cast<uint32_t>(Kernel::MessageId::GenericServiceMeta)) {
                messageLoop();
            }
        }
    }

    bool waitForStatus(uint8_t desiredStatus) {
        uint8_t status {0};
        uint16_t statusRegister {0x64};
        uint32_t timeToWait {10000};

        do {
            asm("inb %1, %0"
                : "=a" (status)
                : "Nd" (statusRegister));
            timeToWait--;
        }
        while (timeToWait > 0 && (status & 1) != desiredStatus);

        return timeToWait == 0;
    }

    enum class Port {
        First,
        Second
    };

    void writeCommand(uint8_t command, Port port) {
        if (port == Port::First) {

            waitForStatus(0);

            uint16_t dataPort {0x60};

            asm("outb %0, %1"
                : //no output
                : "a" (command), "Nd" (dataPort));
        }
        else {
            uint16_t dataPort {0x64};
            uint8_t selectPortTwo {0xD4};

            asm("outb %0, %1"
                : //no output
                : "a" (selectPortTwo), "Nd" (dataPort)); 

            writeCommand(command, Port::First);
        }
    }

    void writeControllerCommand(ControllerCommand command) {
        uint8_t commandByte {static_cast<uint8_t>(command)};
        uint16_t commandPort {0x64};

        asm("outb %0, %1"
            : //no output
            : "a" (commandByte), "Nd" (commandPort));
    }
    
    uint8_t readByte() {
        auto timeout = waitForStatus(1);

        if (timeout) {
            return 0;
        }

        uint16_t dataPort {0x60};

        uint8_t result {0};
        asm("inb %1, %0"
            : "=a" (result)
            : "Nd" (dataPort));

        return result;
    }

    void flushCommandBuffer() {
        uint8_t status {0};
        uint16_t statusRegister {0x64};

        do {
            asm("inb %1, %0"
                : "=a" (status)
                : "Nd" (statusRegister));

            if (status & 1) {
                uint16_t dataPort {0x60};
                uint8_t result {0};
                asm("inb %1, %0"
                    : "=a" (result)
                    : "Nd" (dataPort));
            }
        }
        while ((status & 1)); 
        
    }

    void printPortTestResults(uint8_t testResult) {
        switch(testResult) {
            case static_cast<uint8_t>(PortTestResult::Passed): {
                kprintf(" test passed\n");
                break;
            }
            case static_cast<uint8_t>(PortTestResult::ClockLineStuckLow): {
                kprintf(" clock line stuck low\n");
                break;
            }
            case static_cast<uint8_t>(PortTestResult::ClockLineStuckHigh): {
                kprintf(" clock line stuck high\n");
                break;
            }
            case static_cast<uint8_t>(PortTestResult::DataLineStuckLow): {
                kprintf(" data line stuck low\n");
                break;
            }
            case static_cast<uint8_t>(PortTestResult::DataLineStuckHigh): {
                kprintf(" data line stuck high\n");
                break;
            }
        }
    }

    void testPort(Port port) {
        if (port == Port::First) {
            writeControllerCommand(ControllerCommand::TestFirstPort);
            auto testResult = readByte();

            if (testResult != static_cast<uint32_t>(PortTestResult::Passed)) {
                kprintf("[PS/2] First port test: ");
                printPortTestResults(testResult);
            }
        }
        else {
            writeControllerCommand(ControllerCommand::TestSecondPort);
            auto testResult = readByte();

            if (testResult != static_cast<uint32_t>(PortTestResult::Passed)) {
                kprintf("[PS/2] Second port test: ");
                printPortTestResults(testResult); 
            }
        }
    }

    void initializeController() {
        //skipping steps 1 and 2 from http://wiki.osdev.org/%228042%22_PS/2_Controller

        //disable the devices
        writeControllerCommand(ControllerCommand::DisableFirstPort);
        writeControllerCommand(ControllerCommand::DisableSecondPort);

        //flush the output buffer
        flushCommandBuffer();

        //set the controller configuration byte
        writeControllerCommand(ControllerCommand::GetConfiguration);
        auto config = readByte();

        bool isDualChannel = !(config & (1 << 5));
        config &= ~(1 << 0 | 1 << 1 | 1 << 6);
        writeControllerCommand(ControllerCommand::SetConfiguration);
        writeCommand(config, Port::First);

        //perform controller self test
        writeControllerCommand(ControllerCommand::Test);
        auto testResult = readByte();

        if (testResult != static_cast<uint8_t>(ControllerTestResult::Passed)) {
            kprintf("[PS/2] Controller test failed\n");
        }

        //determine if controller has 1 or 2 channels
        if (!isDualChannel) {
            writeControllerCommand(ControllerCommand::EnableSecondPort);
            writeControllerCommand(ControllerCommand::GetConfiguration);
            auto config = readByte(); 

            if (!(config & (1 << 5))) {
                isDualChannel = true;
                writeControllerCommand(ControllerCommand::DisableSecondPort);
            }
        }

        testPort(Port::First);

        if (isDualChannel) {
            testPort(Port::Second);
        }

        writeControllerCommand(ControllerCommand::EnableFirstPort);
        config = (1 << 2); //system passed post

        if (isDualChannel) {
            writeControllerCommand(ControllerCommand::EnableSecondPort);
            config |= (1 << 1); //enable second port interrupt
        }

        config |= (1 << 0); //enable first port interrupt

        writeControllerCommand(ControllerCommand::SetConfiguration);
        writeCommand(config, Port::First);
    }

    uint16_t identifyDevice(Port port) {
        writeCommand(0xF2, port);
        auto ack = readByte();
        if (ack != 0xFA) {
            return 0;
        }
        auto id = readByte();
        auto id2 = readByte();

        return (id << 8) | id2;
    }

    void identifyDevices() {
        asm volatile("clt");

        //auto firstId = identifyDevice(Port::First);
        //kprintf("[PS/2] First port identified as %x\n", firstId);
        //auto secondId = identifyDevice(Port::Second);
        //kprintf("[PS/2] Second port identified as %x\n", secondId);

        asm volatile("sti");
    }
}