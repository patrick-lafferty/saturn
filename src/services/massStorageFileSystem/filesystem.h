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

    class IBlockDevice {
    public:
        
        IBlockDevice(Partition partition) : partition{partition} {}

        virtual void queueReadSector(uint32_t lba, uint32_t sectorCount) = 0;
        virtual void receiveSector(uint16_t* buffer) = 0;

    protected:

        Partition partition;
    };

    template<typename Read, typename Write>
    struct Requester {

        Requester(Read r, Write w) : read{r}, write{w} {}

        Read read;
        Write write; 
    };

    template<typename Read, typename Write>
    struct Transfer {

        Transfer(Read r, Write w) : read{r}, write{w} {}

        Read read;
        Write write; 
    };

    template<typename Requester, typename Transfer>
    class BlockDevice : public IBlockDevice {
    
    public:

        BlockDevice(Requester requester, Transfer transfer, Partition partition) 
            : IBlockDevice(partition),
                requester{requester},
                transfer{transfer} {
        }
        
        void queueReadSector(uint32_t lba, uint32_t sectorCount) override {
            requester.read(partition.firstLBA + lba, sectorCount);
        }

        virtual void receiveSector(uint16_t* buffer) override {
            transfer.read(buffer);
        }
    
    private:

        Requester requester;
        Transfer transfer;
    };

    class FileSystem {
    public: 

        FileSystem(IBlockDevice* device) : blockDevice{device} {}
        virtual ~FileSystem() {}

        virtual void receiveSector() = 0;

    protected: 
        IBlockDevice* blockDevice;
    };
}