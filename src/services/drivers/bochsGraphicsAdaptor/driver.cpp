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