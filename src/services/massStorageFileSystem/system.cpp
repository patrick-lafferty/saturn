#include "system.h"
#include <services.h>
#include <system_calls.h>
#include <services/virtualFileSystem/virtualFileSystem.h>
#include <vector>
#include <parsing>
#include <services/drivers/ata/driver.h>
#include <services/virtualFileSystem/vostok.h>
#include <crc>
#include <string.h>
#include "ext2/filesystem.h"

using namespace Kernel;
using namespace VFS;
using namespace ATA;
using namespace MassStorageFileSystem::Ext2;

namespace MassStorageFileSystem {

    uint64_t convert64ToLittleEndian(uint64_t input) {
        auto high = convert32ToLittleEndian(input & 0xFFFFFFFF);
        auto low = convert32ToLittleEndian((input >> 32) & 0xFFFFFFFF);

        return (static_cast<uint64_t>(high) << 32) | low;
    }

    uint32_t convert32ToLittleEndian(uint32_t input) {

       return ((input >> 24) & 0xFF)
            | (((input >> 16) & 0xFF) << 8)
            | (((input >> 8) & 0xFF) << 16)
            | ((input & 0xFF) << 24);
    }

    uint16_t convert16ToLittleEndian(uint16_t input) {

       return ((input >> 8) & 0xFF)
            | ((input & 0xFF) << 8);
    }

    GUID::GUID(uint32_t a, uint16_t b, uint16_t c, uint8_t d[8]) {
        this->a = convert32ToLittleEndian(a); 
        this->b = convert16ToLittleEndian(b);
        this->c = convert16ToLittleEndian(c);

        memcpy(this->d, d, sizeof(uint64_t));
    }

    GUID::GUID(uint32_t a, uint16_t b, uint16_t c, uint64_t d) {
        this->a = convert32ToLittleEndian(a); 
        this->b = convert16ToLittleEndian(b);
        this->c = convert16ToLittleEndian(c);

        d = convert64ToLittleEndian(d);

        memcpy(this->d, &d, sizeof(uint64_t));
    }
    
    bool GUID::matches(uint32_t a1, uint16_t b1, uint16_t c1, uint64_t d1) {
        auto other = GUID(a1, b1, c1, d1);

        return a == other.a
                && b == other.b
                && c == other.c
                && memcmp(d, other.d, sizeof(uint64_t)) == 0;
    }

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

    enum class State {
        ReadGPTHeader,
        ReadPartitionTable,
    };

    void MassStorageController::preloop() {

        int remainingPartitionEntries {0};
        State currentState {State::ReadGPTHeader};

        while (true) {
            IPC::MaximumMessageBuffer buffer;
            receive(&buffer);

            if (buffer.messageId == DriverIrqReceived::MessageId) {

                switch(currentState) {
                    case State::ReadGPTHeader: {
                        driver->receiveSector(reinterpret_cast<uint16_t*>(&gptHeader));

                        /*
                        make sure we got a valid GPT header by checking the crc
                        */
                        auto headerCRC = gptHeader.headerCRC32;
                        gptHeader.headerCRC32 = 0;
                        auto ptr = reinterpret_cast<uint8_t*>(&gptHeader);

                        if (!CRC::check32(headerCRC, ptr, sizeof(GPTHeader) - sizeof(GPTHeader::remaining))) {
                            printf("[Mass Storage] Invalid GPT Header, CRC32 check failed\n");
                        }

                        remainingPartitionEntries = gptHeader.partitionEntriesCount;
                        currentState = State::ReadPartitionTable;

                        driver->queueReadSector(gptHeader.partitionArrayLBA);

                        break;
                    }
                    case State::ReadPartitionTable: {

                        uint16_t buffer[256];
                        driver->receiveSector(buffer);

                        Partition partition;
                        memcpy(&partition, buffer, sizeof(Partition));
                        auto type = partition.partitionType;
                        partition.partitionType = GUID(type.a, type.b, type.c, type.d);

                        if (partition.partitionType.matches(0x0FC63DAF, 0x8483, 0x4772, 0x8E793D69D8477DE4)) {
                            fileSystems.push_back(new Ext2FileSystem(partition));
                        }

                        remainingPartitionEntries--;
                       
                        if (remainingPartitionEntries == 0) {
                            return;
                        }

                        break;
                    }
                }
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