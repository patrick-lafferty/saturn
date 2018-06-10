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
#include "mouse.h"
#include <services/virtualFileSystem/vostok.h>

using namespace Kernel;

namespace PS2 {

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

    bool readMouseConfig() {
        sleep(500);

        auto openResult = openSynchronous("/system/hardware/mouse/hasScrollWheel");

        while (!openResult.success) {
            sleep(100);
            openResult = openSynchronous("/system/hardware/mouse/hasScrollWheel"); 
        }

        auto readResult = readSynchronous(openResult.fileDescriptor, 0);

        Vostok::ArgBuffer args{readResult.buffer, sizeof(readResult.buffer)};

        auto type = args.readType();
        bool hasScrollWheel {false};

        if (type == Vostok::ArgTypes::Property) {
            type = args.peekType();

            if (type == Vostok::ArgTypes::Bool) {
                hasScrollWheel = args.read<bool>(type);
            }
        }

        close(openResult.fileDescriptor);
        receiveAndIgnore();

        return hasScrollWheel;
    }

    void messageLoop() {
        KeyboardStates keyboardState {KeyboardStates::Start};
        MouseState mouse {};
        mouse.expectsOptional = readMouseConfig();

        while (true) {
            IPC::MaximumMessageBuffer buffer;
            receive(&buffer);

            switch(buffer.messageNamespace) {
                case IPC::MessageNamespace::PS2: {

                    switch(static_cast<MessageId>(buffer.messageId)) {
                        
                        case MessageId::ReceiveData: {

                            auto message = IPC::extractMessage<ReceiveData>(buffer);

                            if (message.status & 0x20) {
                                transitionMouseState(mouse, message.data);
                            }
                            else {
                                keyboardState = transitionKeyboardState(keyboardState, message.data);
                            }

                            break;
                        }
                        default: {
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

    bool waitForStatus(uint8_t desiredStatus, bool isSet) {
        uint8_t status {0};
        uint16_t statusRegister {0x64};
        uint32_t timeToWait {1000000};

        bool done = false;

        do {
            asm("inb %1, %0"
                : "=a" (status)
                : "Nd" (statusRegister));

            //timeToWait--;

            if (isSet) {
                done = (status & desiredStatus) != 0;
            }
            else {
                done = (status & desiredStatus) == 0;
            }
        }
        while (timeToWait > 0 && !done);

        return timeToWait == 0 && !done;
    }

    void writeCommand(uint8_t command, Port port) {
        if (port == Port::First) {

            if (waitForStatus(2, false)) {
                //timeout, error
                asm("hlt");
            }

            uint16_t dataPort {0x60};

            asm("outb %0, %1"
                : //no output
                : "a" (command), "Nd" (dataPort));
        }
        else {
            uint16_t dataPort {0x64};
            uint8_t selectPortTwo {0xD4};

            if (waitForStatus(2, false)) {
                //timeout, error
                asm("hlt");
            }

            asm("outb %0, %1"
                : //no output
                : "a" (selectPortTwo), "Nd" (dataPort)); 

            writeCommand(command, Port::First);
        }
    }

    void writeControllerCommand(ControllerCommand command) {
        uint8_t commandByte {static_cast<uint8_t>(command)};
        uint16_t commandPort {0x64};

        if (waitForStatus(2, false)) {
            //timeout, error
            asm("hlt");
        }

        asm("outb %0, %1"
            : //no output
            : "a" (commandByte), "Nd" (commandPort));
    }
    
    uint8_t readByte() {

        if (waitForStatus(1, true)) {
            return 0;
        }

        uint16_t dataPort {0x60};

        uint8_t result {0};
        asm("inb %1, %0"
            : "=a" (result)
            : "Nd" (dataPort));

        return result;
    }

    bool waitForAck() {
        return readByte() == ACK;
    }

    void flushCommandBuffer() {
      
        uint16_t dataPort {0x60};
        uint8_t result {0};

        asm("inb %1, %0"
            : "=a" (result)
            : "Nd" (dataPort)); 
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

    void disableController() {

        writeControllerCommand(ControllerCommand::DisableFirstPort);
        writeControllerCommand(ControllerCommand::DisableSecondPort);

        flushCommandBuffer();
    }

    void initializeController() {
        //skipping steps 1 and 2 from http://wiki.osdev.org/%228042%22_PS/2_Controller

        //disable the devices
        disableController();

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

        return;

        writeCommand(0xFF, Port::First);
        auto ack = readByte();

        if (ack != ACK) {
            asm("hlt");
        }

        auto resetResult = readByte();

        if (resetResult != 0xAA) {
            //error
            asm("hlt");
        }

        if (isDualChannel) {
            writeCommand(0xFF, Port::Second);
            ack = readByte();

            if (ack != ACK) {
                asm("hlt");
            }

            resetResult = readByte();

            if (resetResult != 0xAA) {
                asm("hlt");
            }

            resetResult = readByte();

            if (resetResult != 0) {
                asm("hlt");
            }
        }
    }

    uint16_t identifyDevice(Port port) {
        writeCommand(0xF2, port);
        auto ack = readByte();
        if (ack != ACK) {
            return 0;
        }
        auto id = readByte();

        if (id < 5) {
            return id;
        }

        auto id2 = readByte();

        return (id << 8) | id2;
    }

    void identifyDevices() {
        asm volatile("clt");

        //auto firstId = identifyDevice(Port::First);
        auto secondId = identifyDevice(Port::Second);

        if (secondId == static_cast<uint16_t>(DeviceType::StandardMouse)) {
            initializeMouse();
        }

        asm volatile("sti");
    }
}