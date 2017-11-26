#pragma once

#include <stdint.h>
#include <ipc.h>

namespace PS2 {
    void service();
    
    enum class ControllerCommand {
        GetConfiguration = 0x20,
        SetConfiguration = 0x60,
        DisableFirstPort = 0xAD,
        EnableFirstPort = 0xAE,
        DisableSecondPort = 0xA7,
        EnableSecondPort = 0xA8,
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

    void initializeController();
    void identifyDevices();

    struct KeyboardInput : IPC::Message {
        KeyboardInput() {
            messageId = MessageId;
            length = sizeof(KeyboardInput);
        }        

        static uint32_t MessageId;
        uint8_t data;
    };

    struct MouseInput : IPC::Message {
        MouseInput() {
            messageId = MessageId;
            length = sizeof(MouseInput);
        }

        static uint32_t MessageId;
        uint8_t data;
    };
}