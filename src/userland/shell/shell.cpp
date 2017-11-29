#include "shell.h"
#include <services/terminal/terminal.h>
#include <services.h>
#include <system_calls.h>
#include <string.h>
#include <services/memory/memory.h>
#include <memory/physical_memory_manager.h>
#include <stdio.h>
#include <services/virtualFileSystem/virtualFileSystem.h>
#include <services/virtualFileSystem/vostok.h>

namespace Shell {

    void print(const char* s) {
        Terminal::PrintMessage message {};
        message.serviceType = Kernel::ServiceType::Terminal;
        message.stringLength = strlen(s);
        memset(message.buffer, '\0', sizeof(message.buffer));
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

    char* markWord(char* input) {

        if (*input == '\0') {
            return nullptr;
        }

        while (*input != '\0' && *input != ' ') {
            input++;
        }

        *input = '\0';
        return input;
    }

    bool doOpen(char* path, uint32_t& descriptor) {
        open(path);
        IPC::MaximumMessageBuffer buffer;
        receive(&buffer);

        if (buffer.messageId == VFS::OpenResult::MessageId) {
            auto msg = IPC::extractMessage<VFS::OpenResult>(buffer);
            descriptor = msg.fileDescriptor;
            return msg.success;
        }

        return false;
    }

    void doRead(uint32_t descriptor) {
        read(descriptor, 0);
        IPC::MaximumMessageBuffer buffer;
        receive(&buffer);

        if (buffer.messageId == VFS::ReadResult::MessageId) {
            auto r = IPC::extractMessage<VFS::ReadResult>(buffer);
            if (!r.success) return;
            /*char s[20];
            memset(s, '\0', 20);
            memcpy(s, r.buffer, 5);
            print(s);*/
            Vostok::ArgBuffer args{r.buffer, sizeof(r.buffer)};
            print("Signature: (");

            auto type = args.readType();

            while (type != Vostok::ArgTypes::EndArg) {
                auto nextType = args.readType();

                if (nextType == Vostok::ArgTypes::EndArg) {
                    print(") -> ");
                }

                switch(type) {
                    case Vostok::ArgTypes::Void: {
                        print("void");
                        break;
                    }
                    case Vostok::ArgTypes::Uint32: {
                        print("uint32");
                        break;
                    }
                    case Vostok::ArgTypes::Cstring: {
                        print("char*");
                        break;
                    }
                }

                if (nextType != Vostok::ArgTypes::EndArg) {
                    if (args.peekType() != Vostok::ArgTypes::EndArg) {
                        print(", ");
                    }
                }
                else {
                    print("\n");
                }

                type = nextType;                
            }
        }
    }

    bool parse(char* input) {
        auto start = input;
        auto word = markWord(input);

        if (word == nullptr) {
            return false;
        }

        if (strcmp(start, "help") == 0) {
            print("Available commands:\n");
            print("help\n");
            print("version\n");
            print("ask <service name> <request name>\n");
        }
        else if (strcmp(start, "version") == 0) {
            print("Saturn 0.1.0\n");
        }
        else if (strcmp(start, "read") == 0) {
            start = word + 1;
            word = markWord(start);

            if (word != nullptr) {
                uint32_t descriptor {0};
                if (doOpen(start, descriptor)) {
                    doRead(descriptor); 
                }
                else {
                    print("open failed\n");
                }
            }
        }
        else if (strcmp(start, "ask") == 0) {
            start = word + 1;
            word = markWord(start);

            if (word == nullptr) {
                print("available services:\n");
                print("memory\n");
                return true;
            }

            if (strcmp(start, "memory") == 0) {
                start = word + 1;
                word = markWord(start);

                if (word == nullptr) {
                    print("available requests:\n");
                    print("report\n");
                    return true;
                }

                if (strcmp(start, "report") == 0) {
                    Memory::GetPhysicalReport report{};
                    report.serviceType = Kernel::ServiceType::Memory;
                    send(IPC::RecipientType::ServiceName, &report);

                    IPC::MaximumMessageBuffer buffer;
                    receive(&buffer);

                    if (buffer.messageId == Memory::PhysicalReport::MessageId) {
                        auto message = IPC::extractMessage<Memory::PhysicalReport>(buffer);
                        char s[20];
                        memset(s, '\0', sizeof(s));
                        sprintf(s, "Free: %dKB\n", message.freeMemory * Memory::PageSize / 1024);
                        print(s);
                        memset(s, '\0', sizeof(s));
                        sprintf(s, "Total: %dKB\n", message.totalMemory * Memory::PageSize / 1024);
                        print(s);
                    }
                }
                else {
                    return false;
                }
            }
            else {
                return false;
            }
        }
        else {
            return false;
        }

        return true;
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
                printCursor(index + promptLength + 1);

                c = getChar();

                print("\e[38;5;15m");

                switch(c) {
                    case 8: {
                        clearCursor(index + promptLength + 1);

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
                        clearCursor(index + promptLength + 1);
                        print("\n");

                        if (index > 0) {
                        
                            if (!parse(inputBuffer)) {
                                print("Unknown command\n");
                            }
                        }
                        
                        print("\n");

                        memset(inputBuffer, '\0', index);
                        index = 0;

                        break;
                    }
                    default: {
                        clearCursor(index + promptLength + 1);
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