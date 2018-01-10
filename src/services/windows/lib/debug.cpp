#include "debug.h"

namespace Window::Debug {
    void drawBox(uint32_t* buffer, uint32_t positionX, uint32_t positionY, uint32_t width, uint32_t height) {
        for (auto y = positionY; y < positionY + height; y++) {

            for (auto x = positionX; x < positionX + width; x++) {
                buffer[x + y * 800] = 0x00'ff'00'00;

                if (y > positionY && y < (positionY + height - 1)) {
                    x += width - 2;
                }
            }
        }
    }
}