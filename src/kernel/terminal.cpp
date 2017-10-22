#include "terminal.h"
#include "vga.h"

using namespace VGA;

Terminal::Terminal(uint16_t* buffer) {
    this->buffer = buffer;
    auto colour = getColour(Colours::LightBlue, Colours::Black);

    for (uint32_t y = 0; y < Height; y++) {
        for (uint32_t x = 0; x < Width; x++) {
            auto index = y * Width + x;
            buffer[index] = prepareCharacter(' ', colour);
        }
    }
}

void Terminal::writeCharacter(uint8_t character, uint8_t colour) {
    this->buffer[row * Width + column] = prepareCharacter(character, colour);
    column++;
    
    if (column >= Width) {
        column = 0;
        row++;
    }
}