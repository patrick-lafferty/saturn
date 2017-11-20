#include "terminal.h"
#include <string.h>
#include <services.h>
#include <system_calls.h>
#include "vga.h"

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
                uint32_t count {0};

                while (*iterator) {
                    emulator.writeCharacter(*iterator++);
                    count++;
                }

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

    uint32_t Terminal::getIndex() {
        return row * Width + column;
    }

    uint16_t* Terminal::getBuffer() {
        return buffer;
    }
}

