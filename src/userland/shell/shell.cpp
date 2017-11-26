#include "shell.h"
#include <services/terminal/terminal.h>
#include <services.h>
#include <system_calls.h>
#include <string.h>

namespace Shell {

    void print(const char* s) {
        Terminal::PrintMessage message {};
        message.serviceType = Kernel::ServiceType::Terminal;
        message.stringLength = strlen(s);
        memset(message.buffer, 0, sizeof(message.buffer));
        memcpy(message.buffer, s, message.stringLength);
        send(IPC::RecipientType::ServiceName, &message);
    }

    uint8_t getChar() {

        IPC::MaximumMessageBuffer buffer;

        Terminal::GetCharacter message {};
        message.serviceType = Kernel::ServiceType::Terminal;
        send(IPC::RecipientType::ServiceName, &message);
        
        bool done {false};
        while(!done) {
            receive(&buffer);

            if (buffer.messageId == Terminal::CharacterInput::MessageId) {
                done = true;
                auto input = IPC::extractMessage<Terminal::CharacterInput>(buffer);
                return input.character;
            }
        }

        return 0;
    }

    void moveCursor(int column) {
        if (column >= 10) {
            char s[] = {
                '\e',
                '[',
                (char)('0' + column / 10 % 10),
                (char)('0' + column % 10),
                'G',
                '\0'
            };
            print(s);
        }
        else {
            char s[] = {
                '\e',
                '[',
                (char)('0' + column % 10),
                'G',
                '\0'
            };
            print(s);
        }
    }

    void printCursor(int column) {
        moveCursor(column);
        print("\e[48;5;15m \e[48;5;0");
        moveCursor(column);
    }

    void clearCursor(int column) {
        moveCursor(column);
        print("\e[48;5;0m ");
        moveCursor(column);
    }

    int main() {

        char inputBuffer[1000];
        uint32_t index {0};
        auto prompt = "\e[38;5;9mshell> \e[38;5;15m";
        uint32_t promptLength = 7;

        while (true) {
            moveCursor(1);
            print(prompt);

            uint8_t c {0};

            while (c != 13) {
                c = getChar();

                clearCursor(index + promptLength + 1);

                switch(c) {
                    case 8: {
                        if (index > 0) {
                            int a = index + promptLength;
                            moveCursor(a);
                            print(" ");
                            moveCursor(a);

                            index--;
                            printCursor(index + promptLength + 1);
                        }
                        break;
                    }
                    case 13: {
                        print("\n");

                        if (strcmp(inputBuffer, "help") == 0) {
                            print("Available commands:\n");
                            print("help\n");
                            print("version\n");
                        }
                        else if (strcmp(inputBuffer, "version") == 0) {
                            print("Saturn 0.1.0\n");
                        }
                        else {
                            print("Unknown command\n");
                        }
                        
                        print("\n");
                        clearCursor(index + promptLength + 1);

                        memset(inputBuffer, '\0', index);
                        index = 0;

                        break;
                    }
                    default: {
                        char s[] = {(char)c, '\0'};
                        inputBuffer[index] = c;
                        index++;
                        print(s);
                        printCursor(index + promptLength + 1);
                    }
                }

            }
        }
    }
}