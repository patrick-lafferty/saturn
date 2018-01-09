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

        if (buffer.messageNamespace == IPC::MessageNamespace::ServiceRegistry
            && buffer.messageId == static_cast<uint32_t>(Kernel::MessageId::RegisterDriverResult)) {
            auto result = readRegister8(Register::Command);
            resetDevice(Device::Master);
        }
    }

    void Driver::queueReadSector(uint32_t lba, uint32_t sectorCount) {
        /*
        Note: 

        Don't write to the same IO port twice in a row, apparently
        its slower than doing two writes to different ports

        Each register is 8bits wide, some take a 16 bit value, 
        the high byte needs to be written before the low byte
        */
        writeRegister(Register::SectorCount, sectorCount);
        writeRegister(Register::LBALow, lba & 0xFF);
        writeRegister(Register::LBAMid, (lba >> 8) & 0xFF);
        writeRegister(Register::LBAHigh, (lba >> 16) & 0xFF);
        writeRegister(Register::Command, static_cast<uint8_t>(Command::ReadSectors));

        auto result = readRegister8(Register::Control);

        while (isBusy(result)) {
            result = readRegister8(Register::Control);
        }
    }

    bool Driver::receiveSector(uint16_t* buffer) {
        auto result = readRegister8(Register::Command);

        if (isBusy(result)) {
            auto result = readRegister8(Register::Features);
            printf("[ATA] receiveSector failed: busy %d\n", result);
            return false;
        }

        if (hasError(result)) {
            auto result = readRegister8(Register::Features);
            printf("[ATA] receiveSector failed: error %d\n", result);
            return false;
        }

        for(int i = 0; i < 256; i++) {
            buffer[i] = readRegister16(Register::Data);
        }

        return true;
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

        auto result = readRegister8(Register::Control);
        while (isBusy(result)) {
            result = readRegister8(Register::Control);
        }
    }
}