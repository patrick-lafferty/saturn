#include "driver.h"
#include <stdio.h>
#include <services.h>
#include <system_calls.h>

namespace ATA {

    void identifyDevice(Device device) {
        /*
        Identify Device only takes registers:
        -Device [bit 4 == 0 is master, 1 is slave]
        -Command [0xEC]
        */
    }

    void readSector() {
        /*
        Note: 

        Don't write to the same IO port twice in a row, apparently
        its slower than doing two writes to different ports

        Each register is 8bits wide, some take a 16 bit value, 
        the high byte needs to be written before the low byte
        */
    }

    uint8_t readRegister(Register target) {
        uint8_t result;
        uint16_t port {0x1F0};

        port += static_cast<uint16_t>(target);

        asm("inb %1, %0"
            : "=a" (result)
            : "Nd" (port));

        return result;
    }

    Driver::Driver(uint8_t device, uint8_t function) {

        Kernel::RegisterDriver registerRequest;
        registerRequest.type = Kernel::DriverType::ATA;
        send(IPC::RecipientType::ServiceRegistryMailbox, &registerRequest);

        IPC::MaximumMessageBuffer buffer;
        receive(&buffer);

        if (buffer.messageId == Kernel::RegisterDriverResult::MessageId) {
            auto result = readRegister(Register::Command);
        }
    }
}