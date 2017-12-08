#pragma once

#include <stdint.h>

namespace ATA {
    class Driver;
}

namespace MassStorageFileSystem {
    void service();

    struct GUID {
        uint32_t a;
        uint16_t b;
        uint16_t c;
        uint8_t d[8];
    };

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

    struct Partition {

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
    };
}