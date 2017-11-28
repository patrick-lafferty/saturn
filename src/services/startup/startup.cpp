#include "startup.h"
#include <services.h>
#include <system_calls.h>
#include <services/virtualFileSystem/virtualFileSystem.h>

using namespace VFS;

namespace Startup {

    uint32_t openProgram(char* path) {
        open(path);
        IPC::MaximumMessageBuffer buffer;
        receive(&buffer);

        if (buffer.messageId == OpenResult::MessageId) {
            auto message = IPC::extractMessage<OpenResult>(buffer);
            return message.fileDescriptor;
        }

        return 0;
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

        return 0;
    }

    void runProgram(char* path) {
        /*
        until we get an elf loader and an actual filesystem,
        we'll just simulate one using FakeFileSystem  (FFS).
        open(path) just returns a descriptor and does nothing
        read(descriptor) just returns the uintptr_t address
        to the function
        */
        auto descriptor = openProgram(path);

        auto entryPoint = getEntryPoint(descriptor);

        if (entryPoint > 0) {
            run(entryPoint);
        }
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