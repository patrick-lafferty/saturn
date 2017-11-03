#pragma once
#include <stdint.h>

namespace PIC {

    void disable();
    void remapIRQs();
    void maskAllInterrupts();

    /*
    Values taken from http://wiki.osdev.org/8259_PIC
    */
    const uint8_t MasterCommandRegister = 0x20;
    const uint8_t MasterDataRegister = 0x21;
    const uint8_t SlaveCommandRegister = 0xA0;
    const uint8_t SlaveDataRegister = 0xA1;
}