#include "driver.h"
#include <stdio.h>
#include <services.h>
#include <system_calls.h>

namespace ATA {

    uint16_t readRegister16(Register target) {
        uint16_t result;
        uint16_t port {0x1F0};

        port += static_cast<uint16_t>(target);

        asm("inw %1, %0"
            : "=a" (result)
            : "Nd" (port));

        return result;
    }

    uint8_t readRegister8(Register target) {
        uint8_t result;
        uint16_t port {0x1F0};

        port += static_cast<uint16_t>(target);

        asm("inb %1, %0"
            : "=a" (result)
            : "Nd" (port));

        return result;
    }

    void writeRegister(Register target, uint8_t value) {
        uint16_t port {0x1F0};

        port += static_cast<uint16_t>(target);

        asm("outb %0, %1"
            : //no output
            : "a"(value), "Nd" (port));
    }

    bool hasError(uint8_t status) {
        return status & static_cast<uint8_t>(Status::Error);
    }

    bool hasData(uint8_t status) {
        return status & static_cast<uint8_t>(Status::DataRequest);
    }

    bool isBusy(uint8_t status) {
        return status & static_cast<uint8_t>(Status::Busy);
    }

    void wait() {
       uint8_t result {0};

        for (int i = 0; i < 4; i++) {
            result = readRegister8(Register::Control);
        } 
    }

    void reportDeviceIdentification(IdentifyDeviceData& data) {
        printf("[ATA] Device Identification: \n");

        uint16_t* ptr = reinterpret_cast<uint16_t*>(&data);
        for (int i = 0; i < 16; i++) {
            printf("%x %x %x %x %x %x %x %x %x %x %x %x %x %x %x %x ", 
                *ptr++, *ptr++, *ptr++, *ptr++, *ptr++, *ptr++, *ptr++, *ptr++, 
                *ptr++, *ptr++, *ptr++, *ptr++, *ptr++, *ptr++, *ptr++, *ptr++);
        }
    }

    void identifyDevice(Device device) {
        /*
        Identify Device only takes registers:
        -Device [bit 4 == 0 is master, 1 is slave]
        -Command [0xEC]
        */
        writeRegister(Register::SectorCount, 0);
        writeRegister(Register::LBALow, 0);
        writeRegister(Register::LBAMid, 0);
        writeRegister(Register::LBAHigh, 0);
        writeRegister(Register::Command, static_cast<uint8_t>(Command::Identify));

        auto result = readRegister8(Register::Command);

        while (isBusy(result)) {
            result = readRegister8(Register::Command);
        }

        while (!hasData(result) && !hasError(result)) {
            result = readRegister8(Register::Command);
        }

        IdentifyDeviceData data;
        uint16_t* ptr = reinterpret_cast<uint16_t*>(&data);

        for (int i = 0; i < 256; i++) {
            *ptr++ = readRegister16(Register::Data);
        }
    }

    Driver::Driver(uint8_t device, uint8_t function) {

        Kernel::RegisterDriver registerRequest;
        registerRequest.type = Kernel::DriverType::ATA;
        send(IPC::RecipientType::ServiceRegistryMailbox, &registerRequest);

        IPC::MaximumMessageBuffer buffer;
        receive(&buffer);

        if (buffer.messageId == Kernel::RegisterDriverResult::MessageId) {
            auto result = readRegister8(Register::Command);
            resetDevice(Device::Master);
        }
    }

    void Driver::queueReadSector(uint32_t address) {
        //readSector();
        /*
        Note: 

        Don't write to the same IO port twice in a row, apparently
        its slower than doing two writes to different ports

        Each register is 8bits wide, some take a 16 bit value, 
        the high byte needs to be written before the low byte
        */
        writeRegister(Register::SectorCount, 1);
        writeRegister(Register::LBALow, address & 0xFF);
        writeRegister(Register::LBAMid, (address >> 8) & 0xFF);
        writeRegister(Register::LBAHigh, (address >> 16) & 0xFF);
        writeRegister(Register::Command, static_cast<uint8_t>(Command::ReadSectors));
    }

    void Driver::receiveSector(uint16_t* buffer) {
        auto result = readRegister8(Register::Command);

        if (isBusy(result)) {
            printf("[ATA] receiveSector failed: busy\n");
            return;
        }

        if (hasError(result)) {
            printf("[ATA] receiveSector failed: error\n");
            return;
        }

        for(int i = 0; i < 256; i++) {
            *buffer++ = readRegister16(Register::Data);
        }
    }

    void Driver::selectDevice(Device device) {

        if (currentDevice == device) {
            //skip writing to io ports if possible since its expensive
            return;
        }

        writeRegister(Register::Control, 4);
        wait();
        writeRegister(Register::Control, 2);
        wait();

        writeRegister(Register::Device, 0xE0 | static_cast<uint8_t>(device) << 4);
        uint8_t result {0};

        for (int i = 0; i < 5; i++) {
            result = readRegister8(Register::Control);
        }

        currentDevice = Device::Master;
    }

    void Driver::resetDevice(Device device) {
        selectDevice(device);
        writeRegister(Register::Command, static_cast<uint8_t>(Command::Reset));

        writeRegister(Register::Control, 0);
        wait();
    }
}