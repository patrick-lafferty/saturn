#pragma once

#include <stdint.h>

class Terminal {
public:

    Terminal(uint16_t* buffer);
    void writeCharacter(uint8_t character, uint8_t colour);

private:

    uint16_t* buffer;
    uint32_t row {0}, column{0};
};