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
#include <stdlib.h>

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

    VFS::ReadResult readSignature(uint32_t descriptor, bool& success) {
        read(descriptor, 0);
        IPC::MaximumMessageBuffer buffer;
        receive(&buffer);

        if (buffer.messageId == VFS::ReadResult::MessageId) {
            auto r = IPC::extractMessage<VFS::ReadResult>(buffer);
            success = r.success;
            return r;
        }
        else {
            return {};
        }
    }

    void doRead(uint32_t descriptor) {
        bool success {false};
        auto r = readSignature(descriptor, success); 
        Vostok::ArgBuffer args{r.buffer, sizeof(r.buffer)};

        auto type = args.readType();
        if (type == Vostok::ArgTypes::Property) {
            print("Property ");
            //its a property
            type = args.peekType();

            switch(type) {
                case Vostok::ArgTypes::Void: {
                    print("void");
                    return;
                }
                case Vostok::ArgTypes::Uint32: {
                    auto value = args.read<uint32_t>(type);
                    if (!args.readFailed) {
                        char s[15];
                        sprintf(s, "uint32 = %d\n", value);
                        print(s);
                    }
                    break;
                }
                case Vostok::ArgTypes::Cstring: {
                    auto value = args.read<char*>(type);
                    if (!args.readFailed) {
                        char* s = new char[8 + strlen(value)];
                        sprintf(s, "char* = \"%s\"\n", value);
                        print(s);
                        delete s;
                    }
                    break;
                }
            }

        }
        else {
            print("Function Signature: (");
            type = args.readType();

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

    void doWrite(uint32_t descriptor, char* arg) {
        bool success {false};
        auto sig = readSignature(descriptor, success);
        Vostok::ArgBuffer args{sig.buffer, sizeof(sig.buffer)};

        auto expectedType = args.peekType();

        if (expectedType == Vostok::ArgTypes::Uint32) {
            args.write(strtol(arg, nullptr, 10), expectedType);
        }
        else if (expectedType == Vostok::ArgTypes::Bool) {
            bool b {false};
            if (strcmp(arg, "true") == 0) {
                b = true;
            }
            args.write(b, expectedType);
        }

        write(descriptor, sig.buffer, sizeof(sig.buffer));

        /*
        writes to a function yield 1 or 2 messages,
        if the write failed then only a WriteResult,
        otherwise a WriteResult followed by a ReadResult
        */
        {
            IPC::MaximumMessageBuffer buffer;
            receive(&buffer);

            if (buffer.messageId == VFS::WriteResult::MessageId) {
                auto msg = IPC::extractMessage<VFS::WriteResult>(buffer);

                if (!msg.success) {
                    print("write failed\n");
                    return;
                }
            }
        }

        IPC::MaximumMessageBuffer buffer;
        receive(&buffer);

        if (buffer.messageId == VFS::ReadResult::MessageId) {
            auto msg = IPC::extractMessage<VFS::ReadResult>(buffer);

            char s[20];
            memset(s, '\0', sizeof(s));
            memcpy(s, msg.buffer, 10);
            print(s);
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
        else if (strcmp(start, "write") == 0) {
            start = word + 1;
            word = markWord(start);

            if (word != nullptr) {
                auto object = start;
                start = word + 1;

                uint32_t descriptor {0};
                if (doOpen(object, descriptor)) {
                    doWrite(descriptor, start); 
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