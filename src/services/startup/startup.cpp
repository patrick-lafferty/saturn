#include "startup.h"
#include <services.h>
#include <system_calls.h>
#include <services/virtualFileSystem/virtualFileSystem.h>
#include <services/virtualFileSystem/vostok.h>

using namespace VFS;

namespace Startup {

    bool openProgram(char* path, uint32_t& descriptor) {
        open(path);
        IPC::MaximumMessageBuffer buffer;
        receive(&buffer);

        if (buffer.messageId == OpenResult::MessageId) {
            auto message = IPC::extractMessage<OpenResult>(buffer);
            
            if (message.success) {
                descriptor = message.fileDescriptor;
            }

            return message.success;
        }
        else {
            asm ("hlt");
        }

        return false;
    }

    uintptr_t getEntryPoint(uint32_t fileDescriptor) {
        read(fileDescriptor, 4);
        IPC::MaximumMessageBuffer buffer;
        receive(&buffer);

        if (buffer.messageId == ReadResult::MessageId) {
            auto msg = IPC::extractMessage<ReadResult>(buffer);
            uintptr_t result {0};
            memcpy(&result, msg.buffer, sizeof(uintptr_t));

            return result;
        }
        else {
            asm ("hlt");
        }

        return 0;
    }

    bool createProcessObject(uint32_t pid) {
        char path[20];
        memset(path, '\0', sizeof(path));
        sprintf(path, "/process/%d", pid);

        create(path);

        IPC::MaximumMessageBuffer buffer;
        receive(&buffer);

        if (buffer.messageId == VFS::CreateResult::MessageId) {
            auto result = IPC::extractMessage<VFS::CreateResult>(buffer);
            return result.success;
        }
        else {
            asm ("hlt");
        }

        return false;
    }

    void dummyReceive() {
        IPC::MaximumMessageBuffer buffer;
        receive(&buffer);
    }

    void setupProcessObject(char* path, uint32_t pid) {
        char processPath[50];
        memset(processPath, '\0', sizeof(processPath));
        sprintf(processPath, "/process/%d/Executable", pid);
        auto openResult = openSynchronous(processPath);

        if (openResult.success) {
            auto readResult = readSynchronous(openResult.fileDescriptor, 0);

            if (readResult.success) {
                Vostok::ArgBuffer args{readResult.buffer, sizeof(readResult.buffer)};
                auto type = args.readType();

                if (type == Vostok::ArgTypes::Property) {
                    args.write(path, Vostok::ArgTypes::Cstring);
                }

                write(openResult.fileDescriptor, readResult.buffer, sizeof(readResult.buffer));
                dummyReceive();
                close(openResult.fileDescriptor);
                dummyReceive();
            }
        }
        
    }

    void runProgram(char* path) {
        /*
        until we get an elf loader and an actual filesystem,
        we'll just simulate one using FakeFileSystem  (FFS).
        open(path) just returns a descriptor and does nothing
        read(descriptor) just returns the uintptr_t address
        to the function
        */
        uint32_t descriptor {0};

        while (!openProgram(path, descriptor)) {
            asm("hlt");
            sleep(100);
        }

        auto entryPoint = getEntryPoint(descriptor);

        if (entryPoint > 0) {
            auto pid = run(entryPoint);

            while (!createProcessObject(pid)) {
                asm("hlt");
                sleep(100);
            }

            setupProcessObject(path, pid);
        }
        else {
            asm("hlt");
        }

        close(descriptor);
        dummyReceive();
    }


    void service() {
        waitForServiceRegistered(Kernel::ServiceType::VFS);

        runProgram("/bin/vga.service");
        runProgram("/bin/terminal.service");
        runProgram("/bin/ps2.service");
        runProgram("/bin/keyboard.service");
        runProgram("/bin/splash.service");
        runProgram("/bin/shell");
    }
}