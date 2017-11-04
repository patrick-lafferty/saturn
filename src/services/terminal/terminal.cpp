#include "terminal.h"
#include <string.h>

using namespace VGA;

Terminal::Terminal(uint16_t* buffer) {
    this->buffer = buffer;

    bool nightMode = true;

    if (nightMode) {
        defaultColour = getColour(Colours::White, Colours::Black);
    }
    else {
        defaultColour = getColour(Colours::LightBlue, Colours::DarkGray);
    }
    

    for (uint32_t y = 0; y < Height; y++) {
        for (uint32_t x = 0; x < Width; x++) {
            auto index = y * Width + x;
            buffer[index] = prepareCharacter(' ', defaultColour);
        }
    }
}

void Terminal::writeCharacter(uint8_t character) {
    writeCharacter(character, defaultColour);
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

    if (row >= Height) {
        auto byteCount = sizeof(uint16_t) * Width * (Height - 1);
        memcpy(buffer, buffer + Width, byteCount);

        row = Height - 1;

        for (uint32_t x = 0; x < Width; x++) {
            auto index = row * Width + x;
            buffer[index] = prepareCharacter(' ', defaultColour);
        }
    }
}