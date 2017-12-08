#pragma once

#include <stdint.h>

namespace MassStorageFileSystem {

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

    struct Partition {
        GUID partitionType;
        GUID id;
        uint64_t firstLBA;
        uint64_t lastLBA;
        uint64_t attributeFlags;
        uint8_t name[72];
    };

    class FileSystem {
    public: 

        FileSystem(Partition partition) : partition{partition} {}
        virtual ~FileSystem() {}

    protected: 

        Partition partition;
    };
}