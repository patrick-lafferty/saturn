#include <stdint.h>

void printString(const char* message, int line, int column) {

    auto position = line * 80 + column;
    auto buffer = reinterpret_cast<uint16_t*>(0xb8000) + position;

    while(message && *message != '\0') {
        *buffer++ = *message++ | (0xF << 8);
    }
}

[[noreturn]]
extern "C"
void main() {
    printString("Inside kernel", 0, 0);
}