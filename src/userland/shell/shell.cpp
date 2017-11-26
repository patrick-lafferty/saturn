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

    int main() {

        char inputBuffer[1000];
        uint32_t index {0};

        while (true) {
            print("\e[38;5;9mshell> \e[38;5;15m");

            uint8_t c {0};

            while (c != 13) {
                c = getChar();

                switch(c) {
                    case 8: {
                        if (index > 0) {
                            index--;
                            //print("\e[1;");
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
                        
                        print("\n");

                        memset(inputBuffer, '\0', index);
                        index = 0;

                        break;
                    }
                    default: {
                        char s[] = {(char)c, '\0'};
                        inputBuffer[index] = c;
                        index++;
                        print(s);
                    }
                }
            }
        }
    }
}