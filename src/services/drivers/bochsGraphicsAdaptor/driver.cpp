#include "driver.h"
#include <services.h>
#include <system_calls.h>

using namespace Kernel;

namespace BGA {

    void registerMessages() {

    }

    void messageLoop(uint32_t address) {

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

        if (buffer.messageId == RegisterServiceDenied::MessageId) {
            //can't print to screen, how to notify?
        }
        else if (buffer.messageId == VGAServiceMeta::MessageId) {
            registerMessages();

            NotifyServiceReady ready;
            send(IPC::RecipientType::ServiceRegistryMailbox, &ready);

            //auto vgaMeta = IPC::extractMessage<VGAServiceMeta>(buffer);

            //return vgaMeta.vgaAddress;
            return 0;
        }

        return 0;
    }

    void setupAdaptor() {
        uint16_t ioPort {0x01CE};
        uint16_t dataPort {0x01CF};

        uint16_t indexEnable {4};
        asm volatile("outw %%ax, %%dx"
            : //no output
            : "a" (indexEnable), "d" (ioPort));

    }

    void service() {
        auto bgaAddress = registerService();
        setupAdaptor();
        messageLoop(bgaAddress);
    }
}