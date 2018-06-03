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
#pragma once

#include <stdint.h>
#include <ipc.h>

namespace PS2 {

    void service();

    enum class Port {
        First,
        Second
    };

    void writeCommand(uint8_t command, Port port);
    uint8_t readByte();
    bool waitForAck();
    
    enum class ControllerCommand {
        GetConfiguration = 0x20,
        SetConfiguration = 0x60, //no response
        DisableFirstPort = 0xAD, //no response
        EnableFirstPort = 0xAE, //no response
        DisableSecondPort = 0xA7, //no response
        EnableSecondPort = 0xA8, //no response
        Test = 0xAA,
        TestFirstPort = 0xAB,
        TestSecondPort = 0xA9
    };

    enum class ControllerTestResult {
        Passed = 0x55,
        Failed = 0xFC
    };

    enum class PortTestResult {
        Passed = 0x0,
        ClockLineStuckLow = 0x01,
        ClockLineStuckHigh = 0x02,
        DataLineStuckLow = 0x03,
        DataLineStuckHigh = 0x04
    };

    inline const uint8_t ACK {0xFA};

    enum class PhysicalKey : uint8_t {
        A = 0x1C,
        B = 0x32,
        C = 0x21,
        D = 0x23,
        E = 0x24,
        F = 0x2B,
        G = 0x34,
        H = 0x33,
        I = 0x43,
        J = 0x3B,
        K = 0x42, 
        L = 0x4B,
        M = 0x3A,
        N = 0x31,
        O = 0x44,
        P = 0x4D,
        Q = 0x15,
        R = 0x2D,
        S = 0x1B,
        T = 0x2C,
        U = 0x3C,
        V = 0x2A,
        W = 0x1D,
        X = 0x22,
        Y = 0x35,
        Z = 0x1A,

        Zero = 0x45,
        One = 0x16,
        Two = 0x1E,
        Three = 0x26,
        Four = 0x25,
        Five = 0x2E,
        Six = 0x36,
        Seven = 0x3D,
        Eight = 0x3E,
        Nine = 0x46,

        F1 = 0x05,
        F2 = 0x06,
        F3 = 0x04,
        F4 = 0x0C,
        F5 = 0x03,
        F6 = 0x0B,
        F7 = 0x83,
        F8 = 0x0A,
        F9 = 0x01,
        F10 = 0x09,
        F11 = 0x78,
        F12 = 0x07,

        Keypad0 = 0x70,
        Keypad1 = 0x69,
        Keypad2 = 0x72,
        Keypad3 = 0x7A,
        Keypad4 = 0x6B,
        Keypad5 = 0x73,
        Keypad6 = 0x74, 
        Keypad7 = 0x6c,
        Keypad8 = 0x75,
        Keypad9 = 0x7D,
        KeypadPeriod = 0x71,
        KeypadPlus = 0x79,
        KeypadMinus = 0x7B,
        KeypadTimes = 0x7C,

        Tab = 0x0D,
        BackTick = 0x0E,
        LeftAlt = 0x11,
        LeftShift = 0x12,
        LeftControl = 0x14,
        Space = 0x29,
        Comma = 0x41,
        Period = 0x49,
        ForwardSlash = 0x4A,
        Semicolon = 0x4C, 
        Minus = 0x4E,
        SingleQuote = 0x52,
        LeftBracket = 0x54,
        Equal = 0x55,
        CapsLock = 0x58,
        RightShift = 0x59,
        Enter = 0x5A,
        RightBracket = 0x5B,
        BackSlash = 0x5D,
        Backspace = 0x66,
        Escape = 0x76,
        NumberLock = 0x77,
        ScrollLock = 0x7E,

    };

    enum class KeyStatus {
        Pressed,
        Released
    };

    void initializeController();
    void identifyDevices();

    enum class DeviceType {
        StandardMouse = 0,
        ScrollMouse = 0x3,
        FiveButtonMouse = 0x4,
        KeyboardWithTranslation = 0xAB41,
        KeyboardWithTranslationAlt = 0xABC1,
        Keyboard = 0xAB83
    };

    enum class MessageId {
        KeyboardInput,
        MouseInput,
        ReceiveData
    };

    struct ReceiveData : IPC::Message {
        ReceiveData() {
            messageId = static_cast<uint32_t>(MessageId::ReceiveData);
            length = sizeof(ReceiveData);
            messageNamespace = IPC::MessageNamespace::PS2;
        }        
        
        uint8_t status;
        uint8_t data;
    };
}