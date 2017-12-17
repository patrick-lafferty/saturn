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
#include <parsing>

using namespace std;

namespace Shell {

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

    void printCursor(int column) {
        printf("\e[%dG\e[48;5;15m \e[48;5;0m\e[%dG", column, column);
    }

    void clearCursor(int column, char c = ' ') {
        if (c == '\0') {
            c = ' ';
        }

        printf("\e[%dG\e[48;5;0m%c\e[%dG", column, c, column);
    }

    bool doOpen(string_view path, uint32_t& descriptor) {
        char pathNullTerminated[64];
        path.copy(pathNullTerminated, path.length());
        pathNullTerminated[path.length()] = '\0';

        open(pathNullTerminated);
        IPC::MaximumMessageBuffer buffer;
        receive(&buffer);

        if (buffer.messageId == VirtualFileSystem::OpenResult::MessageId) {
            auto msg = IPC::extractMessage<VirtualFileSystem::OpenResult>(buffer);
            descriptor = msg.fileDescriptor;

            return msg.success;
        }

        return false;
    }

    VirtualFileSystem::ReadResult readSignature(uint32_t descriptor, bool& success) {
        read(descriptor, 0);
        IPC::MaximumMessageBuffer buffer;
        receive(&buffer);

        if (buffer.messageId == VirtualFileSystem::ReadResult::MessageId) {
            auto r = IPC::extractMessage<VirtualFileSystem::ReadResult>(buffer);
            success = r.success;
            return r;
        }
        else {
            return {};
        }
    }

    void printResult(Vostok::ArgBuffer& args) {
        auto type = args.readType();
        if (type == Vostok::ArgTypes::Property) {
            printf("Property ");

            while (type != Vostok::ArgTypes::EndArg) {

                type = args.peekType();

                switch(type) {
                    case Vostok::ArgTypes::Void: {
                        printf("void");
                        return;
                    }
                    case Vostok::ArgTypes::Uint32: {
                        auto value = args.read<uint32_t>(type);
                        if (!args.hasErrors()) {
                            printf("uint32 = %u\n", value);
                        }
                        break;
                    }
                    case Vostok::ArgTypes::Cstring: {
                        auto value = args.read<char*>(type);
                        if (!args.hasErrors()) {
                            printf("char* = \"%s\"\n", value);
                        }
                        break;
                    }
                    case Vostok::ArgTypes::EndArg: {
                        return;
                    }
                }
            }
        }
        else {
            printf("Function Signature: (");
            type = args.readType();

            while (type != Vostok::ArgTypes::EndArg) {
                auto nextType = args.readType();

                if (nextType == Vostok::ArgTypes::EndArg) {
                    printf(") -> ");
                }

                switch(type) {
                    case Vostok::ArgTypes::Void: {
                        printf("void");
                        break;
                    }
                    case Vostok::ArgTypes::Uint32: {
                        printf("uint32");
                        break;
                    }
                    case Vostok::ArgTypes::Cstring: {
                        printf("char*");
                        break;
                    }
                }

                if (nextType != Vostok::ArgTypes::EndArg) {
                    if (args.peekType() != Vostok::ArgTypes::EndArg) {
                        printf(", ");
                    }
                }
                else {
                    printf("\n");
                }

                type = nextType;                
            }
        }
    }

    void doReadVostok(uint32_t descriptor) {
        bool success {false};
        auto r = readSignature(descriptor, success); 
        Vostok::ArgBuffer args{r.buffer, sizeof(r.buffer)};
        printResult(args);        
    }

    void doReadDirectory(uint32_t descriptor) {
        while (true) {
            read(descriptor, 0);
            IPC::MaximumMessageBuffer buffer;
            receive(&buffer);

            if (buffer.messageId == VirtualFileSystem::ReadResult::MessageId) {
                auto result = IPC::extractMessage<VirtualFileSystem::ReadResult>(buffer);

                if (!result.success || result.bytesWritten == 0) {
                    return;
                }

                auto ptr = result.buffer;

                for (auto i = 0u; i < sizeof(result.buffer); i++) { 
                    auto index = *reinterpret_cast<uint32_t*>(ptr);

                    if (index == 0) {
                        return;
                    }

                    ptr += 4;
                    auto type = *ptr;
                    ptr++;
                    auto name = reinterpret_cast<char*>(ptr);
                    printf("%s\n", name);
                    ptr += strlen(name) + 1;
                }
            }
            else {
                return;
            }
        }
    }

    void doReadFile(uint32_t descriptor) {
         while (true) {
            read(descriptor, 0);
            IPC::MaximumMessageBuffer buffer;
            receive(&buffer);

            if (buffer.messageId == VirtualFileSystem::Read512Result::MessageId) {
                auto result = IPC::extractMessage<VirtualFileSystem::Read512Result>(buffer);

                if (!result.success || result.bytesWritten == 0) {
                    return;
                }

                char* buffer = new char[result.bytesWritten + 1];
                memcpy(buffer, result.buffer, result.bytesWritten);
                buffer[result.bytesWritten] = '\0';

                printf("%s", buffer);
                
            }
            else {
                return;
            }
         }
    }

    void doWriteFunction_impl(uint32_t descriptor, VirtualFileSystem::ReadResult& sig) {
        write(descriptor, sig.buffer, sizeof(sig.buffer));

        /*
        writes to a function yield 1 or 2 messages,
        if the write failed then only a WriteResult,
        otherwise a WriteResult followed by a ReadResult
        */
        {
            IPC::MaximumMessageBuffer buffer;
            receive(&buffer);

            if (buffer.messageId == VirtualFileSystem::WriteResult::MessageId) {
                auto msg = IPC::extractMessage<VirtualFileSystem::WriteResult>(buffer);

                if (!msg.success) {
                    printf("write failed\n");
                    return;
                }
            }
        }

        IPC::MaximumMessageBuffer buffer;
        receive(&buffer);

        if (buffer.messageId == VirtualFileSystem::ReadResult::MessageId) {
            auto msg = IPC::extractMessage<VirtualFileSystem::ReadResult>(buffer);
            Vostok::ArgBuffer args{msg.buffer, sizeof(msg.buffer)};
            printResult(args); 
        }
    }

    void doWriteProperty_impl(uint32_t descriptor, VirtualFileSystem::ReadResult& sig) {
        write(descriptor, sig.buffer, sizeof(sig.buffer));

        IPC::MaximumMessageBuffer buffer;
        receive(&buffer);

        if (buffer.messageId == VirtualFileSystem::WriteResult::MessageId) {
            auto msg = IPC::extractMessage<VirtualFileSystem::WriteResult>(buffer);

            if (!msg.success) {
                printf("write failed\n");
                return;
            }
        }
    }

    void doWriteVostok(uint32_t descriptor) {
        bool success {false};
        auto sig = readSignature(descriptor, success);
        Vostok::ArgBuffer args{sig.buffer, sizeof(sig.buffer)};
        //type starts with function or property
        auto type = args.readType();

        if (type == Vostok::ArgTypes::Function) {
            auto expectedType = args.peekType();

            if (expectedType != Vostok::ArgTypes::Void) {
                printf("[Shell] function expects arguments\n");
                return;
            }

            doWriteFunction_impl(descriptor, sig);
        }
        else if (type == Vostok::ArgTypes::Property) {
            printf("[Shell] property expects value\n");
        }
    }

    void doWriteVostok(uint32_t descriptor, std::vector<string_view> arg, int startIndex) {
        bool success {false};
        auto sig = readSignature(descriptor, success);
        Vostok::ArgBuffer args{sig.buffer, sizeof(sig.buffer)};
        //type starts with function or property
        auto type = args.readType();

        if (type == Vostok::ArgTypes::Function) {
            auto expectedType = args.peekType();

            while (expectedType != Vostok::ArgTypes::Void
                && expectedType != Vostok::ArgTypes::EndArg) {

                auto value = arg[startIndex];
                
                if (expectedType == Vostok::ArgTypes::Uint32) {
                    char argTerminated[value.length() + 1];
                    value.copy(argTerminated, value.length());
                    argTerminated[value.length()] = '\0';
                    args.write(strtol(argTerminated, nullptr, 10), expectedType);
                    startIndex++;
                }
                else if (expectedType == Vostok::ArgTypes::Bool) {
                    bool b {false};

                    if (value.compare("true") == 0) {
                        b = true;
                    }

                    args.write(b, expectedType);
                }

                expectedType = args.readType();
            }

            doWriteFunction_impl(descriptor, sig); 
        }
        else if (type == Vostok::ArgTypes::Property) {
            auto expectedType = args.peekType();

            if (expectedType == Vostok::ArgTypes::Uint32) {
                char argTerminated[arg[startIndex].length() + 1];
                arg[startIndex].copy(argTerminated, arg[startIndex].length());
                argTerminated[arg[startIndex].length()] = '\0';
                args.write(strtol(argTerminated, nullptr, 10), expectedType);
            }
            else if (expectedType == Vostok::ArgTypes::Bool) {
                bool b {false};
                if (arg[startIndex].compare("true") == 0) {
                    b = true;
                }
                args.write(b, expectedType);
            }

            doWriteProperty_impl(descriptor, sig);            
        }
    }

    bool parse(char* input) {

        auto words = split({input, strlen(input)}, ' ');
        
        if (words.empty()) {
            return false;
        }

        if (words[0].compare("help") == 0) {
            printf("Available commands:\nhelp\nversion\nread\nwrite\n");
        }
        else if (words[0].compare("version") == 0) {
            printf("Saturn 0.1.0\n");
        }
        else if (words[0].compare("get") == 0) {

            if (words.size() < 2) {
                return false;
            }

            uint32_t descriptor {0};
            if (doOpen(words[1], descriptor)) {
                doReadVostok(descriptor); 
            }
            else {
                printf("open failed\n");
            }
        }
        else if (words[0].compare("write") == 0) {

            if (words.size() < 2) {
                return false;
            }

            uint32_t descriptor {0};
            if (doOpen(words[1], descriptor)) {
                if (words.size() > 2) {
                    doWriteVostok(descriptor, words, 2); 
                }
                else {
                    doWriteVostok(descriptor);
                }
            }
            else {
                printf("open failed\n");
            }
        }
        else if (words[0].compare("list") == 0) {
            if (words.size() < 2) {
                return false;
            }

            uint32_t descriptor {0};

            if (doOpen(words[1], descriptor)) {
                doReadDirectory(descriptor);
            }
        }
        else if (words[0].compare("read") == 0) {
            if (words.size() < 2) {
                return false;
            }

            uint32_t descriptor {0};

            if (doOpen(words[1], descriptor)) {
                doReadFile(descriptor);
            }
        }
        else {
            return false;
        }

        return true;
    }

    void moveCursor(int column) {
        printf("\e[%dG", column);
    }

    int main() {

        char inputBuffer[500];
        char previousLine[500];
        memset(inputBuffer, '\0', sizeof(inputBuffer));
        uint32_t index {0};
        uint32_t cursorPosition {0};
        uint32_t promptLength = 7;

        while (true) {
            moveCursor(1);
            printf("\e[38;5;9mshell> \e[38;5;15m");

            uint8_t c {0};

            while (c != 13) {
                printCursor(cursorPosition + promptLength + 1);

                c = getChar();

                printf("\e[38;5;15m");

                switch(c) {
                    case 8: {
                        //backspace
                        clearCursor(cursorPosition + promptLength + 1);

                        if (index != cursorPosition) {
                            memmove(inputBuffer + cursorPosition, inputBuffer + cursorPosition + 1, index - cursorPosition);
                        }

                        inputBuffer[index] = '\0';

                        if (index > 0) {
                            int a = cursorPosition + promptLength;
                            clearCursor(a);

                            index--;
                            cursorPosition--;
                            printCursor(cursorPosition + promptLength + 1);
                        }
                        break;
                    }
                    case 13: {
                        //newline
                        clearCursor(cursorPosition + promptLength + 1, inputBuffer[cursorPosition]);
                        printf("\n");

                        if (index > 0) {
                            if (!parse(inputBuffer)) {
                                printf("Unknown command\n");
                            }
                        }
                        
                        printf("\n");
                        memcpy(previousLine, inputBuffer, sizeof(previousLine));
                        memset(inputBuffer, '\0', sizeof(inputBuffer));
                        index = 0;
                        cursorPosition = 0;

                        break;
                    }
                    case 14: {
                        //left arrow
                        if (cursorPosition > 0) {
                            auto c = index == cursorPosition ? ' ' : inputBuffer[cursorPosition];
                            clearCursor(cursorPosition + promptLength + 1, c);
                            cursorPosition--;
                        }

                        break;
                    }
                    case 15: {
                        //up arrow
                        memcpy(inputBuffer, previousLine, sizeof(previousLine));
                        moveCursor(1 + promptLength);
                        printf("%s", inputBuffer); 
                        auto length = strlen(inputBuffer);
                        moveCursor(1 + promptLength + length);
                        index = length;
                        cursorPosition = length;
                        break;
                    }
                    case 16: {
                        break;
                    }
                    case 17: {
                        //right arrow
                        if (cursorPosition < index) {
                            auto c = index == cursorPosition ? ' ' : inputBuffer[cursorPosition];
                            clearCursor(cursorPosition + promptLength + 1, c);
                            cursorPosition++;
                        }
                        break;
                    }
                    default: {
                        clearCursor(cursorPosition + promptLength + 1);
                        if (index == cursorPosition) {
                            inputBuffer[index] = c;
                            printf("%c", c);
                        }
                        else {
                            memmove(inputBuffer + cursorPosition + 1, inputBuffer + cursorPosition, index - cursorPosition);
                            inputBuffer[cursorPosition] = c;
                            moveCursor(1 + promptLength);
                            printf("%s", inputBuffer);
                        }

                        index++;
                        cursorPosition++;

                        printCursor(cursorPosition + promptLength + 1);
                    }
                }

            }
        }
    }
}