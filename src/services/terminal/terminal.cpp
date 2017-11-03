#include "terminal.h"
#include <string.h>

using namespace VGA;

Terminal::Terminal(uint16_t* buffer) {
    this->buffer = buffer;
    auto colour = getColour(Colours::LightBlue, Colours::DarkGray);

    for (uint32_t y = 0; y < Height; y++) {
        for (uint32_t x = 0; x < Width; x++) {
            auto index = y * Width + x;
            buffer[index] = prepareCharacter(' ', colour);
        }
    }
}

void Terminal::writeCharacter(uint8_t character, uint8_t colour) {
    if (character == '\n') {
        column = 0;
        row++;
    }
    else {
        this->buffer[row * Width + column] = prepareCharacter(character, colour);
        column++;
    }

    if (column >= Width) {
        column = 0;
        row++;
    }

    if (row > Height) {
        auto offset = sizeof(uint16_t) * Width * (Height - 1);
        memcpy(buffer, buffer + Width, offset);
        memset(buffer + offset, 0, offset);
        row--;
    }
}