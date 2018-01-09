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
#include "driver.h"
#include <services.h>
#include <system_calls.h>
#include <services/startup/startup.h>

using namespace Kernel;

namespace BGA {

    void messageLoop(uint32_t address) {
        
        Startup::runProgram("/bin/windows.service");

        while (true) {
            IPC::MaximumMessageBuffer buffer;
            receive(&buffer);
        }
    }

    uint32_t registerService() {
        RegisterService registerRequest {};
        registerRequest.type = ServiceType::BGA;

        IPC::MaximumMessageBuffer buffer;

        send(IPC::RecipientType::ServiceRegistryMailbox, &registerRequest);
        receive(&buffer);

        if (buffer.messageNamespace == IPC::MessageNamespace::ServiceRegistry) {

            if (buffer.messageId == static_cast<uint32_t>(MessageId::RegisterServiceDenied)) {
                //can't print to screen, how to notify?
            }

            else if (buffer.messageId == static_cast<uint32_t>(MessageId::VGAServiceMeta)) {
                auto msg = IPC::extractMessage<VGAServiceMeta>(buffer);

                NotifyServiceReady ready;
                send(IPC::RecipientType::ServiceRegistryMailbox, &ready);

                return msg.vgaAddress;
            }
        }

        return 0;
    }

    enum class BGAIndex {
        Id,
        XRes,
        YRes,
        BPP,
        Enable,
        Bank,
        VirtWidth,
        VirtHeight,
        XOffset,
        YOffset
    };

    void writeRegister(BGAIndex bgaIndex, uint16_t value) {
        uint16_t ioPort {0x01CE};
        uint16_t dataPort {0x01CF};
       
        auto index = static_cast<uint16_t>(bgaIndex);
        asm volatile("outw %%ax, %%dx"
            : //no output
            : "a" (index), "d" (ioPort));

        asm volatile("outw %%ax, %%dx"
            : //no output
            : "a" (value), "d" (dataPort));
    }

    uint16_t readRegister(BGAIndex bgaIndex) {
        uint16_t ioPort {0x01CE};
        uint16_t dataPort {0x01CF};
       
        auto index = static_cast<uint16_t>(bgaIndex);
        asm volatile("outw %%ax, %%dx"
            : //no output
            : "a" (index), "d" (ioPort));

        uint16_t result {0};

        asm volatile("inw %1, %0"
            : "=a" (result)
            : "Nd" (dataPort));

        return result;
    }

    void discover() {
        auto id = readRegister(BGAIndex::Id);

        /*
        QEMU reports 0xB0C0
        VirtualBox reports 0xB0C4
        */
        printf("[BGA] Id: %d\n", id);
    }

    void setupAdaptor() {
        writeRegister(BGAIndex::Enable, 0);

        writeRegister(BGAIndex::XRes, 800);
        writeRegister(BGAIndex::YRes, 600);
        writeRegister(BGAIndex::BPP, 32);

        writeRegister(BGAIndex::Enable, 1 | 0x40);
    }

    void service() {
        auto bgaAddress = registerService();
        sleep(200);
        setupAdaptor();

        messageLoop(bgaAddress);
    }
}