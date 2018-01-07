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

using namespace VirtualFileSystem;
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
        const char* path = "/";
        memcpy(request.path, path, strlen(path) + 1);
        request.serviceType = Kernel::ServiceType::VFS;
        request.cacheable = true;

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

        if (buffer.messageNamespace == IPC::MessageNamespace::VFS
            && buffer.messageId != static_cast<uint32_t>(MessageId::ReadResult)) {
            return nullptr;
        }

        auto callResult = IPC::extractMessage<ReadResult>(buffer);
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

            switch (buffer.messageNamespace) {
                case IPC::MessageNamespace::ServiceRegistry: {

                    switch (static_cast<Kernel::MessageId>(buffer.messageId)) {
                        case Kernel::MessageId::DriverIrqReceived: {
                            if (queuedRequests.empty()) {
                                printf("[MassStorageController] Received DriverIrq but have no queuedRequests\n");
                                continue;
                            }

                            if (pendingCommand == PendingDiskCommand::Read) {
                                auto& request = queuedRequests.front();

                                fileSystems[request.requesterId]->receiveSector();
                                request.sectorCount--;

                                if (request.sectorCount == 0) {
                                    pendingCommand = PendingDiskCommand::None;
                                    queuedRequests.pop();

                                    if (!queuedRequests.empty()) {
                                        queueReadSector(queuedRequests.front());
                                    }
                                }
                            }
                            break;
                        }
                        default:
                            break;
                    }
                    break;
                }
                case IPC::MessageNamespace::VFS: {

                    switch (static_cast<MessageId>(buffer.messageId)) {
                        case MessageId::GetDirectoryEntries: {
                            auto request = IPC::extractMessage<GetDirectoryEntries>(buffer);
                            handleGetDirectoryEntries(request);
                            break;
                        }
                        case MessageId::OpenRequest: {
                            auto request = IPC::extractMessage<OpenRequest>(buffer);
                            OpenResult result;
                            result.success = true;
                            result.serviceType = Kernel::ServiceType::VFS;
                            result.fileDescriptor = fileSystems[0]->openFile(request.index, request.requestId);
                            result.requestId = request.requestId;
                            send(IPC::RecipientType::ServiceName, &result);
                            break;
                        }
                        case MessageId::ReadRequest: {
                            auto request = IPC::extractMessage<::VirtualFileSystem::ReadRequest>(buffer);
                            handleReadRequest(request);
                            break;
                        }
                        case MessageId::SeekRequest: {
                            auto request = IPC::extractMessage<::VirtualFileSystem::SeekRequest>(buffer);
                            handleSeekRequest(request);
                            break;
                        }
                        default:
                            break;
                    }
                    break;
                }
                default:
                    break;
            }
        }
    }

    enum class State {
        ReadGPTHeader,
        ReadPartitionTable,
    };

    void MassStorageController::preloop() {

        int remainingPartitionEntries {0};
        State currentState {State::ReadGPTHeader};
        sleep(400);

        while (true) {
            IPC::MaximumMessageBuffer buffer;
            receive(&buffer);

            switch (buffer.messageNamespace) {
                case IPC::MessageNamespace::ServiceRegistry: {

                    switch (static_cast<Kernel::MessageId>(buffer.messageId)) {
                        case Kernel::MessageId::DriverIrqReceived: {
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
                                        asm("hlt");
                                    }

                                    remainingPartitionEntries = gptHeader.partitionEntriesCount;
                                    currentState = State::ReadPartitionTable;

                                    driver->queueReadSector(gptHeader.partitionArrayLBA, 1);

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
                                        auto requesterId = fileSystems.size();
                                        pendingCommand = PendingDiskCommand::None;

                                        auto requester = makeRequester(
                                            [this, requesterId](auto lba, auto sectorCount) {
                                                queueReadSectorRequest(lba, sectorCount, requesterId);
                                            },
                                            []() {return;}
                                        );

                                        auto transfer = makeTransfer(
                                            [this](auto buffer) {
                                                return driver->receiveSector(buffer);
                                            },
                                            []() {return;}
                                        );

                                        auto device = new BlockDevice(
                                            requester,
                                            transfer,
                                            partition
                                        );
                                        fileSystems.push_back(new Ext2FileSystem(device));
                                    }
                                    else {
                                        printf("[Mass Storage] PartitionType doesn't match\n");
                                        asm("hlt");
                                    }

                                    return;

                                    remainingPartitionEntries--;
                                
                                    if (remainingPartitionEntries == 0) {
                                        return;
                                    }

                                    break;
                                }
                            }

                            break;
                        }
                        default:
                            break;
                    }
                    break;
                }
                default:
                    break;
            }
        }
    }

    void MassStorageController::queueReadSectorRequest(uint32_t lba, uint32_t sectorCount, uint32_t requesterId) {
        Request request{lba, sectorCount, requesterId};
        queuedRequests.push(request);

        if (pendingCommand == PendingDiskCommand::None) {
            queueReadSector(request);
        }
    }

    void MassStorageController::queueReadSector(Request request) {
        pendingCommand = PendingDiskCommand::Read;
        driver->queueReadSector(request.lba, request.sectorCount);
    }

    void MassStorageController::handleGetDirectoryEntries(GetDirectoryEntries& request) {
        /*
        TODO: for now assume fileSystems[0] is the only mount
        */

        fileSystems[0]->readDirectory(request.index, request.requestId);
    }

    void MassStorageController::handleReadRequest(::VirtualFileSystem::ReadRequest& request) {
        /*
        TODO: for now assume fileSystems[0] is the only mount
        */
        fileSystems[0]->readFile(request.fileDescriptor, request.requestId, request.readLength);
    }

    void MassStorageController::handleSeekRequest(::VirtualFileSystem::SeekRequest& request) {
        /*
        TODO: for now assume fileSystems[0] is the only mount
        */
        fileSystems[0]->seekFile(request.fileDescriptor, request.requestId, request.offset, static_cast<Origin>(request.origin));
    }

    void service() {
        waitForServiceRegistered(Kernel::ServiceType::VFS);
        registerService();
        sleep(300);
        auto driver = setupDriver();
        auto massStorage = new MassStorageController(driver);
        massStorage->preloop();
        massStorage->messageLoop();
    }

    MassStorageController::MassStorageController(Driver* driver) {
        this->driver = driver;
        driver->queueReadSector(1, 1);
    }
}