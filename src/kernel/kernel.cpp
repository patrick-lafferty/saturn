#include "vga.h"
#include "terminal.h"

extern "C" int kernel_main() {
    Terminal terminal{reinterpret_cast<uint16_t*>(0xB8000)};
    terminal.writeCharacter('P', getColour(VGA::Colours::DarkBlue, VGA::Colours::LightMagenta));
    terminal.writeCharacter('P', getColour(VGA::Colours::DarkBlue, VGA::Colours::LightMagenta));
    terminal.writeCharacter('P', getColour(VGA::Colours::DarkBlue, VGA::Colours::LightMagenta));
    terminal.writeCharacter('P', getColour(VGA::Colours::DarkBlue, VGA::Colours::LightMagenta));
    terminal.writeCharacter('P', getColour(VGA::Colours::DarkBlue, VGA::Colours::LightMagenta));
    terminal.writeCharacter('P', getColour(VGA::Colours::DarkBlue, VGA::Colours::LightMagenta));
    terminal.writeCharacter('P', getColour(VGA::Colours::DarkBlue, VGA::Colours::LightMagenta));
    terminal.writeCharacter('P', getColour(VGA::Colours::DarkBlue, VGA::Colours::LightMagenta));
    terminal.writeCharacter('P', getColour(VGA::Colours::DarkBlue, VGA::Colours::LightMagenta));
    terminal.writeCharacter('P', getColour(VGA::Colours::DarkBlue, VGA::Colours::LightMagenta));
    terminal.writeCharacter('P', getColour(VGA::Colours::DarkBlue, VGA::Colours::LightMagenta));
    terminal.writeCharacter('P', getColour(VGA::Colours::DarkBlue, VGA::Colours::LightMagenta));
    terminal.writeCharacter('P', getColour(VGA::Colours::DarkBlue, VGA::Colours::LightMagenta));

    return 0;
}