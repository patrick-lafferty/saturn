#include "terminal.h"
#include <string.h>
#include <services.h>
#include <system_calls.h>
#include "vga.h"
#include <stdlib.h>

using namespace VGA;
using namespace Kernel;

namespace Terminal {

    uint32_t PrintMessage::MessageId;

    void registerMessages() {
        IPC::registerMessage<PrintMessage>();
    }

    void messageLoop() {

        Terminal emulator{new uint16_t[2000]};
        
        while (true) {
            IPC::MaximumMessageBuffer buffer;
            receive(&buffer);

            if (buffer.messageId == PrintMessage::MessageId) {
                auto message = IPC::extractMessage<PrintMessage>(buffer);
                auto iterator = message.buffer;
                auto index = emulator.getIndex();

                /*while (*iterator) {
                    emulator.writeCharacter(*iterator++);
                    count++;
                }*/
                auto dirty = emulator.interpret(iterator, message.length);
                //uint32_t count {dirty.endIndex - dirty.startIndex};
                uint32_t count{dirty.endIndex};
                //count /= sizeof(uint16_t);
                BlitMessage blit;
                blit.index = index;
                blit.count = count;
                blit.serviceType = ServiceType::VGA;
                memcpy(blit.buffer, emulator.getBuffer() + index, sizeof(uint16_t) * count);

                send(IPC::RecipientType::ServiceName, &blit);
            }
        }
    }

    void service() {
        RegisterService registerRequest {};
        registerRequest.type = ServiceType::Terminal;

        IPC::MaximumMessageBuffer buffer;

        send(IPC::RecipientType::ServiceRegistryMailbox, &registerRequest);
        receive(&buffer);

        if (buffer.messageId == RegisterServiceDenied::MessageId) {
            //TODO: how show error if print service can't print
            //print(100, static_cast<int>(ServiceType::Terminal));
        }
        else if (buffer.messageId == TerminalServiceMeta::MessageId) {
            //print(101, static_cast<int>(ServiceType::Terminal));
            registerMessages();
            messageLoop();
        }
    }

    Terminal::Terminal(uint16_t* buffer) {
        this->buffer = buffer;

        bool nightMode = true;

        if (nightMode) {
            currentColour = getColour(Colours::White, Colours::Black);
        }
        else {
            currentColour = getColour(Colours::LightBlue, Colours::DarkGray);
        }

        for (uint32_t y = 0; y < Height; y++) {
            for (uint32_t x = 0; x < Width; x++) {
                auto index = y * Width + x;
                buffer[index] = prepareCharacter(' ', currentColour);
            }
        }
    }

    void Terminal::writeCharacter(uint8_t character) {
        writeCharacter(character, currentColour);
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
                buffer[index] = prepareCharacter(' ', currentColour);
            }
        }
    }

    //enum class

    uint32_t Terminal::handleSelectGraphicRendition(char* buffer, uint32_t length) {
        auto start = buffer;
        char* end;
        auto code = strtol(start, &end, 10);

        switch (code) {
            case 0: {
                break;
            }
            case 38: {
                buffer = end;

                if (*buffer == ';') {
                    auto arg1 = strtol(buffer + 1, &end, 10);

                    if (arg1 == 5) {
                        buffer = end;
                        
                        if (*buffer == ';') {
                            auto arg2 = strtol(buffer + 1, &end, 10);

                            if (end == buffer + 1) {
                                //parse failure
                            }
                            else {
                                setForegroundColour(currentColour, arg2);
                            }
                        }
                    }
                    else if (arg1 == 2) {

                    }
                    else {
                        //error
                    }
                }
                break;
            }
        }

        return end - start + 1; //ends SGR ends with 'm', consume that
    } 

    uint32_t Terminal::handleEscapeSequence(char* buffer, uint32_t count) {
        auto start = buffer;

        switch (*buffer++) {
            case 'O': {
                //f1-f4...
                break;
            }
            case '[': {

                auto sequenceStart = buffer;
                bool validSequence {false};

                /*
                According to https://en.wikipedia.org/wiki/ANSI_escape_code,
                CSI is x*y*z, where:
                * is the kleene start
                x is a parameter byte from 0x30 to 0x3F
                y is an intermediate byte from 0x20 to 0x2F
                z is the final byte from 0x40 to 0x7E, and determines the sequence
                */

                while (count > 0) {
                    auto next = *buffer;

                    if (next >= 0x40 && next <= 0x7E) {
                        validSequence = true;
                        break;
                    }

                    buffer++;
                    count--;
                }

                if (!validSequence) {
                    return buffer - start;
                }

                switch(*buffer) {
                    case static_cast<char>(CSIFinalByte::SelectGraphicRendition): {
                        return 1 + handleSelectGraphicRendition(sequenceStart, buffer - sequenceStart);
                    }
                }

                break;
            }
        }

        return buffer - start;
    }

    DirtyRect Terminal::interpret(char* buffer, uint32_t count) {
        DirtyRect dirty {};
        dirty.startIndex = getIndex();

        while (count > 0 && *buffer != 0) {
            count--;

            switch(*buffer) {
                case 27: {
                    if (count == 0) {
                        break;
                    }

                    auto consumedChars = 1 + handleEscapeSequence(buffer + 1, count);
                    buffer += consumedChars;
                    count -= consumedChars;
                    break;
                }
                default: {
                    writeCharacter(*buffer);
                    dirty.endIndex++;
                    buffer++;
                    break;
                }
            }

        }

        return dirty;
    }

    uint32_t Terminal::getIndex() {
        return row * Width + column;
    }

    uint16_t* Terminal::getBuffer() {
        return buffer;
    }
}

