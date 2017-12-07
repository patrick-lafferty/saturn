#include "system.h"
#include <services.h>
#include <system_calls.h>
#include <services/virtualFileSystem/virtualFileSystem.h>
#include <vector>
#include <parsing>
#include <services/drivers/ata/driver.h>
#include <services/virtualFileSystem/vostok.h>

using namespace Kernel;
using namespace VFS;
using namespace ATA;

namespace Ext2FileSystem {

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

    Driver* setup() {

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

    void messageLoop(Driver* driver) {

        while (true) {
            IPC::MaximumMessageBuffer buffer;
            receive(&buffer);

        }
    }

    void service() {
        waitForServiceRegistered(ServiceType::VFS);
        registerService();
        sleep(100);
        auto driver = setup();
        messageLoop(driver);
    }
}