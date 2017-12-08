#pragma once

#include <stdint.h>
#include <vector>

namespace ATA {
    class Driver;
}

namespace MassStorageFileSystem {
    void service();

    uint64_t convert64ToLittleEndian(uint64_t input);
    uint32_t convert32ToLittleEndian(uint32_t input);
    uint16_t convert16ToLittleEndian(uint16_t input);

    struct GUID {
        GUID() {}
        GUID(uint32_t a, uint16_t b, uint16_t c, uint8_t d[8]);
        GUID(uint32_t a, uint16_t b, uint16_t c, uint64_t d);
        bool matches(uint32_t a1, uint16_t b1, uint16_t c1, uint64_t d1);
            
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
        GUID partitionType;
        GUID id;
        uint64_t firstLBA;
        uint64_t lastLBA;
        uint64_t attributeFlags;
        uint8_t name[72];
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
    };
}