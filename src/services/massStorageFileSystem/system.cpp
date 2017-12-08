#include "system.h"
#include <services.h>
#include <system_calls.h>
#include <services/virtualFileSystem/virtualFileSystem.h>
#include <vector>
#include <parsing>
#include <services/drivers/ata/driver.h>
#include <services/virtualFileSystem/vostok.h>
#include <crc>

using namespace Kernel;
using namespace VFS;
using namespace ATA;

namespace MassStorageFileSystem {

    void registerService() {
        MountRequest request;
        const char* path = "/test";
        memcpy(request.path, path, strlen(path) + 1);
        request.serviceType = ServiceType::VFS;

        send(IPC::RecipientType::ServiceName, &request);
    }

    uint32_t callFind() {

        auto openResult = openSynchronous("/system/hardware/pci/find");

        if (!openResult.success) {
            return -1;
        }

        auto readResult = readSynchronous(openResult.fileDescriptor, 0);

        if (!readResult.success) {
            return -1;
        } 
        
        Vostok::ArgBuffer args{readResult.buffer, sizeof(readResult.buffer)};
        auto type = args.readType();

        if (type != Vostok::ArgTypes::Function) {
            return -1;
        }

        args.write(1, Vostok::ArgTypes::Uint32);
        type = args.readType();
        args.write(1, Vostok::ArgTypes::Uint32);

        auto writeResult = writeSynchronous(openResult.fileDescriptor, readResult.buffer, sizeof(readResult.buffer));

        if (!writeResult.success) {
            return -1;
        }

        return openResult.fileDescriptor;
    }

    Driver* setupDriver() {

        auto descriptor = callFind();

        if (descriptor == -1) {
            return nullptr;
        }

        IPC::MaximumMessageBuffer buffer;
        receive(&buffer);

        if (buffer.messageId != VFS::ReadResult::MessageId) {
            return nullptr;
        }

        auto callResult = IPC::extractMessage<VFS::ReadResult>(buffer);
        auto args = Vostok::ArgBuffer{callResult.buffer, sizeof(callResult.buffer)};
        auto type = args.readType();

        if (type != Vostok::ArgTypes::Property) {
            return nullptr;
        }

        auto combinedId = args.read<uint32_t>(args.peekType());
        uint8_t functionId = combinedId & 0xFF;
        uint8_t deviceId = (combinedId >> 8) & 0xFF;

        return new Driver(deviceId, functionId);
    }

    void MassStorageController::messageLoop() {
        while (true) {
            IPC::MaximumMessageBuffer buffer;
            receive(&buffer);
        }
    }

    void MassStorageController::preloop() {
        while (true) {
            IPC::MaximumMessageBuffer buffer;
            receive(&buffer);

            if (buffer.messageId == DriverIrqReceived::MessageId) {
                driver->receiveSector(reinterpret_cast<uint16_t*>(&gptHeader));

sleep(2000);
                auto s = gptHeader.signature;

                printf("%x %x %x %x %x %x %x %x\n",
                    s[0], 
                    s[1], 
                    s[2], 
                    s[3], 
                    s[4], 
                    s[5], 
                    s[6], 
                    s[7]);

                printf("%x %x\n", gptHeader.headerSize, gptHeader.headerCRC32);

                /*
                make sure we got a valid GPT header by checking the crc
                */
                auto headerCRC = gptHeader.headerCRC32;
                gptHeader.headerCRC32 = 0;
                auto ptr = reinterpret_cast<uint8_t*>(&gptHeader);

                if (!CRC::check32(headerCRC, ptr, sizeof(GPTHeader) - sizeof(GPTHeader::remaining))) {
                    printf("[Mass Storage] Invalid GPT Header, CRC32 check failed\n");
                }

                return;
            }
        }
    }

    void service() {
        waitForServiceRegistered(ServiceType::VFS);
        registerService();
        sleep(100);
        auto driver = setupDriver();
        auto massStorage = new MassStorageController(driver);
        massStorage->preloop();
        massStorage->messageLoop();
    }

    MassStorageController::MassStorageController(Driver* driver) {
        this->driver = driver;
        driver->queueReadSector(1);
    }
}