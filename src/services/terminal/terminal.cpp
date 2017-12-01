#include "terminal.h"
#include <string.h>
#include <services.h>
#include <system_calls.h>
#include "vga.h"
#include <stdlib.h>
#include <algorithm>

using namespace VGA;
using namespace Kernel;

namespace Terminal {

    uint32_t PrintMessage::MessageId;
    uint32_t KeyPress::MessageId;
    uint32_t GetCharacter::MessageId;
    uint32_t CharacterInput::MessageId;

    void registerMessages() {
        IPC::registerMessage<PrintMessage>();
        IPC::registerMessage<KeyPress>();
        IPC::registerMessage<GetCharacter>();
        IPC::registerMessage<CharacterInput>();
    }
    
    class InputQueue {
    public:
        
        void add(char c) {
            buffer[lastWrite] = c;
            lastWrite++;

            if (lastWrite >= maxLength) {
                lastWrite = 0;
            }

            size++;

            if (size >= maxLength) {
                size = maxLength;
            }
        }

        bool inputIsAvailable() {
            return size > 0;
        }

        char get() {
            auto c = buffer[lastRead];
            lastRead++;
            size--;

            if (lastRead >= maxLength) {
                lastRead = 0;
            }

            return c;
        }

    private:
        char buffer[128];
        const uint32_t maxLength {128};
        uint32_t lastRead {0};
        uint32_t lastWrite {0};
        uint32_t size {0};
    };

    void messageLoop() {

        Terminal emulator{new uint16_t[2000]};
        InputQueue queue;
        uint32_t taskIdWaitingForInput {0};
        
        while (true) {
            IPC::MaximumMessageBuffer buffer;
            receive(&buffer);

            if (buffer.messageId == PrintMessage::MessageId) {
                auto message = IPC::extractMessage<PrintMessage>(buffer);
                auto iterator = message.buffer;
                auto index = emulator.getIndex();
                auto dirty = emulator.interpret(iterator, message.stringLength);

                if (dirty.overflowed) {
                    ScrollScreen scroll {};
                    scroll.serviceType = ServiceType::VGA;
                    send(IPC::RecipientType::ServiceName, &scroll);
                }

                index = dirty.startIndex;
                uint32_t count{dirty.endIndex};

                uint32_t blitBufferSize = sizeof(BlitMessage::buffer) / sizeof(uint16_t);                
                auto blitsToDo = count / blitBufferSize + 1;

                for (auto i = 0u; i < blitsToDo; i++) {

                    BlitMessage blit;
                    blit.index = index + i * blitBufferSize;
                    blit.count = std::min(count, blitBufferSize);
                    blit.serviceType = ServiceType::VGA;
                    memcpy(blit.buffer, 
                        emulator.getBuffer() + index + i * blitBufferSize, 
                        sizeof(uint16_t) * blit.count);

                    send(IPC::RecipientType::ServiceName, &blit);

                    count -= blitBufferSize;
                }
                
            }
            else if (buffer.messageId == KeyPress::MessageId) {
                auto message = IPC::extractMessage<KeyPress>(buffer);
                queue.add(message.key);

                if (taskIdWaitingForInput != 0) {
                    CharacterInput input {};
                    input.character = queue.get();
                    input.recipientId = taskIdWaitingForInput;
                    send(IPC::RecipientType::TaskId, &input);
                    taskIdWaitingForInput = 0;    
                }
            }
            else if (buffer.messageId == GetCharacter::MessageId) {
                if (queue.inputIsAvailable()) {
                    CharacterInput input {};
                    input.character = queue.get();
                    input.recipientId = buffer.senderTaskId;
                    send(IPC::RecipientType::TaskId, &input);
                }
                else {
                    taskIdWaitingForInput = buffer.senderTaskId;
                }
            }
        }
    }

    void registerService() {
        waitForServiceRegistered(ServiceType::VGA);

        RegisterService registerRequest {};
        registerRequest.type = ServiceType::Terminal;

        IPC::MaximumMessageBuffer buffer;

        send(IPC::RecipientType::ServiceRegistryMailbox, &registerRequest);
        receive(&buffer);

        if (buffer.messageId == RegisterServiceDenied::MessageId) {
            //TODO: how show error if print service can't print
        }
        else if (buffer.messageId == GenericServiceMeta::MessageId) {
            registerMessages();
        }
    }

    void service() {
        registerService();
        messageLoop();
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
            dirty.overflowed = true;
            auto byteCount = sizeof(uint16_t) * Width * (Height - 1);
            memcpy(buffer, buffer + Width, byteCount);

            row = Height - 2;
            dirty.startIndex = getIndex();
            row = Height - 1;

            for (uint32_t x = 0; x < Width; x++) {
                auto index = row * Width + x;
                buffer[index] = prepareCharacter(' ', currentColour);
            }

        }
    }
    
    uint32_t Terminal::handleCursorPosition(char* buffer, [[maybe_unused]] uint32_t length) {
        auto start = buffer;
        char* end {nullptr};

        if (*buffer == ';') {
            //row was omitted
        }
        else {
            uint32_t newRow = strtol(start, &end, 10);
            uint32_t newColumn {1};
            buffer = end;

            if (*buffer == ';') {
                buffer++;

                if (*buffer == 'H') {
                    //column omitted
                }
                else {
                    newColumn = strtol(buffer, &end, 10);

                    if (end == buffer + 1) {
                        //TODO: parse failure
                    }
                    else {

                        row = newRow - 1;
                        column = newColumn - 1;

                        //if (newRow * Width + newColumn != dirty.startIndex) {
                            dirty.startIndex = getIndex();                            
                        //}
                    }
                }
            }
            else {
                //TODO: error
            }
        }

        return end - start + 1;
    }

    uint32_t Terminal::handleCursorHorizontalAbsolute(char* buffer, [[maybe_unused]] uint32_t length) {
        auto start = buffer;
        char* end {nullptr};

        if (*buffer == 'G') {
            //column was omitted
            column = 0;
        }
        else {
            uint32_t newColumn = strtol(start, &end, 10);
            column = newColumn - 1;
        }

        dirty.startIndex = getIndex();

        return end - start + 1;
    }

    uint32_t Terminal::handleSelectGraphicRendition(char* buffer, [[maybe_unused]] uint32_t length) {
        auto start = buffer;
        char* end {nullptr};
        auto code = strtol(start, &end, 10);

        switch (code) {
            case 0: {
                //TODO: stub
                break;
            }
            case 38:
            case 48: {
                buffer = end;

                if (*buffer == ';') {
                    auto arg1 = strtol(buffer + 1, &end, 10);

                    if (arg1 == 5) {
                        buffer = end;
                        
                        if (*buffer == ';') {
                            auto arg2 = strtol(buffer + 1, &end, 10);

                            if (end == buffer + 1) {
                                //TODO: parse failure
                            }
                            else {
                                if (code == 38) {
                                    setForegroundColour(currentColour, arg2);
                                }
                                else {
                                    setBackgroundColour(currentColour, arg2);
                                }
                            }
                        }
                    }
                    else if (arg1 == 2) {

                    }
                    else {
                        //TODO: error
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
                    case static_cast<char>(CSIFinalByte::CursorPosition): {
                        return 1 + handleCursorPosition(sequenceStart, buffer - sequenceStart);
                    }
                    case static_cast<char>(CSIFinalByte::CursorHorizontalAbsolute): {
                        return 1 + handleCursorHorizontalAbsolute(sequenceStart, buffer - sequenceStart);
                    }
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
        dirty.startIndex = getIndex();
        dirty.endIndex = 0;
        dirty.overflowed = false;

        while (count >= 0 && *buffer != 0) {
            count--;

            switch(*buffer) {
                case 27: {
                    if (count == 0) {
                        break;
                    }

                    auto consumedChars = 1 + handleEscapeSequence(buffer + 1, count);
                    buffer += consumedChars;
                    count -= consumedChars;

                    //TODO: better way of tracking dirty areas
                    /*auto index = getIndex();

                    if (index != dirty.startIndex) {
                        dirty.startIndex = index;
                    }*/

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

        dirty.endIndex = getIndex() - dirty.startIndex;

        return dirty;
    }

    uint32_t Terminal::getIndex() {
        return row * Width + column;
    }

    uint16_t* Terminal::getBuffer() {
        return buffer;
    }
}

