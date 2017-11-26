#include "shell.h"
#include <services/terminal/terminal.h>
#include <services.h>
#include <system_calls.h>

namespace Shell {

    void print(char* s) {
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
        while (true) {
            print("shell> ");

            uint8_t c {0};

            while (c != 13) {
                c = getChar();

                switch(c) {
                    case 13: {
                        print("\n");
                        break;
                    }
                    default: {
                        char s[] = {(char)c, '\0'};
                        print(s);
                    }
                }
            }
        }
    }
}