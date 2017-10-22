#pragma once

#include <stdint.h>
#include "vga.h"

class Terminal {
public:

    Terminal(uint16_t* buffer);
    void writeCharacter(uint8_t character, uint8_t colour = getColour(VGA::Colours::LightBlue, VGA::Colours::DarkGray));

    static Terminal& getInstance() {
        static Terminal instance{reinterpret_cast<uint16_t*>(0xB8000)};
        return instance;
    }

private:

    uint16_t* buffer;
    uint32_t row {0}, column{0};
};

