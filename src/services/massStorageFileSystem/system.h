#pragma once

#include <stdint.h>
#include <vector>
#include "filesystem.h"

namespace ATA {
    class Driver;
}

namespace MassStorageFileSystem {
    void service();

    uint64_t convert64ToLittleEndian(uint64_t input);
    uint32_t convert32ToLittleEndian(uint32_t input);
    uint16_t convert16ToLittleEndian(uint16_t input);

    struct GPTHeader {
        uint8_t signature[8];
        uint32_t revision;
        uint32_t headerSize;
        uint32_t headerCRC32;
        uint32_t reserved;
        uint64_t currentLBA;
        uint64_t backupLBA;
        uint64_t firstUsableLBA;
        uint64_t lastUsableLBA;
        GUID diskGUID; 
        uint64_t partitionArrayLBA;
        uint32_t partitionEntriesCount;
        uint32_t partitionEntrySize;
        uint32_t partitionArrayCRC;
        uint8_t remaining[164];
    };

    enum class PendingDiskCommand {
        Read,
        Write,
        None
    };

    class MassStorageController {
    public:

        MassStorageController(ATA::Driver* driver);
        void preloop();
        void messageLoop();

    private:

        ATA::Driver* driver;
        GPTHeader gptHeader;
        PendingDiskCommand pendingCommand {PendingDiskCommand::None};
        std::vector<Partition> partitions;
        std::vector<FileSystem*> fileSystems;
    };
}