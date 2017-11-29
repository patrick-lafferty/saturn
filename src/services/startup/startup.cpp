#include "startup.h"
#include <services.h>
#include <system_calls.h>
#include <services/virtualFileSystem/virtualFileSystem.h>

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

        return false;
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
            sleep(100);
        }

        auto entryPoint = getEntryPoint(descriptor);

        if (entryPoint > 0) {
            auto pid = run(entryPoint);

            while (!createProcessObject(pid)) {
                sleep(100);
            }
        }

        close(descriptor);
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