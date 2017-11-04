#pragma once

#include <stdint.h>
#include "vga.h"

class Terminal {
public:

    Terminal(uint16_t* buffer);
    void writeCharacter(uint8_t character);
    void writeCharacter(uint8_t character, uint8_t colour);

    static Terminal& getInstance() {
        static Terminal instance{reinterpret_cast<uint16_t*>(0xB8000)};
        return instance;
    }

private:

    uint16_t* buffer;
    uint32_t row {0}, column{0};
    uint8_t defaultColour;
};

