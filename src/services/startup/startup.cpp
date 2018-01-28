/*
Copyright (c) 2017, Patrick Lafferty
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

    * Redistributions of source code must retain the above copyright
      notice, this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright
      notice, this list of conditions and the following disclaimer in the
      documentation and/or other materials provided with the distribution.
    * Neither the name of the copyright holder nor the names of its 
      contributors may be used to endorse or promote products derived from 
      this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR 
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/
#include "startup.h"
#include <services.h>
#include <system_calls.h>
#include <services/virtualFileSystem/virtualFileSystem.h>
#include <services/virtualFileSystem/vostok.h>
#include <saturn/wait.h>

using namespace VirtualFileSystem;

namespace Startup {

    bool openProgram(const char* path, uint32_t& descriptor) {
        open(path);
        IPC::MaximumMessageBuffer buffer;
        filteredReceive(&buffer, IPC::MessageNamespace::VFS, static_cast<uint32_t>(MessageId::OpenResult));

        auto message = IPC::extractMessage<OpenResult>(buffer);
        
        if (message.success) {
            descriptor = message.fileDescriptor;
        }

        return message.success;
    }

    uintptr_t getEntryPoint(uint32_t fileDescriptor) {
        read(fileDescriptor, 4);
        IPC::MaximumMessageBuffer buffer;
        filteredReceive(&buffer, IPC::MessageNamespace::VFS, static_cast<uint32_t>(MessageId::ReadResult));

        auto msg = IPC::extractMessage<ReadResult>(buffer);
        uintptr_t result {0};
        memcpy(&result, msg.buffer, sizeof(uintptr_t));

        return result;
}

    bool createProcessObject(uint32_t pid) {
        char path[20];
        memset(path, '\0', sizeof(path));
        sprintf(path, "/process/%d", pid);

        create(path);

        IPC::MaximumMessageBuffer buffer;
        filteredReceive(&buffer, IPC::MessageNamespace::VFS, static_cast<uint32_t>(MessageId::CreateResult));

        auto result = IPC::extractMessage<CreateResult>(buffer);
        return result.success;
    }

    void dummyReceive() {
        IPC::MaximumMessageBuffer buffer;
        receive(&buffer);
    }

    void setupProcessObject(const char* path, uint32_t pid) {
        char processPath[50];
        memset(processPath, '\0', sizeof(processPath));
        sprintf(processPath, "/process/%d/executable", pid);
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
            }

            close(openResult.fileDescriptor);
            dummyReceive();
        }
    }

    void runProgram(const char* path) {
        /*
        until we get an elf loader and an actual filesystem,
        we'll just simulate one using FakeFileSystem  (FFS).
        open(path) just returns a descriptor and does nothing
        read(descriptor) just returns the uintptr_t address
        to the function
        */
        uint32_t descriptor {0};

        while (!openProgram(path, descriptor)) {
            sleep(10);
        }

        auto entryPoint = getEntryPoint(descriptor);

        if (entryPoint > 0) {
            auto pid = run(entryPoint);

            while (!createProcessObject(pid)) {
                sleep(10);
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
        Saturn::Event::waitForMount("/process");

        runProgram("/bin/vga.service");
        runProgram("/bin/terminal.service");
        runProgram("/bin/ps2.service");
        runProgram("/bin/keyboard.service");
        //runProgram("/bin/shell");
    }
}