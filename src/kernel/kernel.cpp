#include <services/terminal/vga.h>
#include <services/terminal/terminal.h>
#include <stdio.h>

extern "C" int kernel_main() {
    Terminal terminal{reinterpret_cast<uint16_t*>(0xB8000)};
    auto colour = getColour(VGA::Colours::LightBlue, VGA::Colours::DarkGray);

    auto f = [&](auto c) {
        terminal.writeCharacter(c, colour);
    };

    //f('P');
    //f('a');
    //f('t');
    f(printf(nullptr, 0));

    return 0;
}