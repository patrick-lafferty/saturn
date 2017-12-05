#include "rtc.h"

namespace RTC {

    void selectIndex(uint8_t index) {
        uint16_t rtcIndexPort {0x70};

        asm("out %0, %1"
            : //no output
            : "a" (index), "Nd" (rtcIndexPort));
    }

    void writePort(uint8_t value) {
        uint16_t rtcIOPort {0x71};

        asm("out %0, %1"
            : //no output
            : "a" (value), "Nd" (rtcIOPort));
    }

    uint8_t readPort() {
        uint8_t result;
        uint16_t rtcIOPort {0x71};

        asm("inb %1, %0"
            : "=a" (result)
            : "Nd" (rtcIOPort));

        return result;
    }

    void enable(uint8_t rate) {
        uint8_t statusRegisterB {0};

        selectIndex(0x8B);
        statusRegisterB = readPort();
        selectIndex(0x8B);

        statusRegisterB |= 0x40;

        writePort(statusRegisterB);

        selectIndex(0x8A);
        auto statusRegisterA = readPort();
        selectIndex(0x8A);
        /*
        0xD corresponds to a rate of 13
        frequency is calculated as:
        32768 >> (rate - 1)

        so this gives a rate of 8Hz
        */
        writePort((statusRegisterA & 0xF0) | rate);
    }

    void disable() {
        uint8_t statusRegisterB {0};

        selectIndex(0x8B);
        statusRegisterB = readPort();
        selectIndex(0x8B);
        statusRegisterB &= ~0x40;
        writePort(statusRegisterB);
    }

    void reset() {
        selectIndex(0xC);
        readPort();
    }
}