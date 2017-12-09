#pragma once

#include "../filesystem.h"

namespace MassStorageFileSystem::Ext2 {
    
    struct SuperBlock {
        uint32_t totalInodes;
        uint32_t totalBlocks;
        uint32_t superUserBlocksReserved;
        uint32_t totalAllocatedBlocks;
        uint32_t totalUnallocatedBlocks;
        uint32_t superblockIndex;
        uint32_t log2BlockSize;
        uint32_t log2FragmentSize;
        uint32_t blocksPerGroup;
        uint32_t fragmentsPerGroup;
        uint32_t inodesPerGroup;
        uint32_t lastMountTime;
        uint32_t lastWrittenTime;
        uint16_t mountsSinceLastCheck;
        uint16_t mountsUntilConsistencyCheck;
        uint16_t signature;
        uint16_t state;
        uint16_t errorAction;
        uint16_t minorVersion;
        uint32_t timeOfLastCheck;
        uint32_t intervalBetweenMandatoryChecks;
        uint32_t operatingSystemId;
        uint32_t majorVersion;
        uint16_t userId;
        uint16_t groupId;
    };

    enum class States {
        Clean = 1,
        Error = 2
    };

    enum class ErrorAction {
        Ignore = 1,
        RemountReadonly = 2,
        Panic = 3
    };

    enum class OperatingSystemId {
        Linux,
        Hurd,
        MASIX,
        FreeBSD,
        Other
    };

    enum class ReadState {
        SuperBlock,
        File,
        None
    };

    struct BlockGroupDescriptor {
        uint32_t blockBitmapId;
        uint32_t inodeBitmapId;
        uint32_t inodeTableId;
        uint16_t unallocatedBlocks;
        uint16_t unallocatedInodes;
        uint16_t directories;
    };

    struct Inode {
        uint16_t typeAndPermissions;
        uint16_t userId;
        uint32_t sizeLower32Bits;
        uint32_t lastAccessTime;
        uint32_t createtionTime;
        uint32_t lastModificationTime;
        uint32_t deletionTime;
        uint16_t groupId;
        uint16_t hardLinkCount;
        uint32_t diskSectorsCount;
        uint32_t flags;
        uint32_t operatingSystem;
        uint32_t directBlock0;
        uint32_t directBlock1;
        uint32_t directBlock2;
        uint32_t directBlock3;
        uint32_t directBlock4;
        uint32_t directBlock5;
        uint32_t directBlock6;
        uint32_t directBlock7;
        uint32_t directBlock8;
        uint32_t directBlock9;
        uint32_t directBlock10;
        uint32_t directBlock11;
        uint32_t singlyIndirectBlock;
        uint32_t doublyIndirectBlock;
        uint32_t triplyIndirectBlock;
        uint32_t generationNumber;
        uint32_t extendedAttributes;
        uint32_t sizeUpper32Bits;
        uint32_t fragmentBlockId;
        uint8_t osSpecificValue[12];
    };

    class Ext2FileSystem : public FileSystem {
    public:
        Ext2FileSystem(IBlockDevice* device); 
        void receiveSector() override;

    private:

        SuperBlock superBlock;
        ReadState readState;
    };
}