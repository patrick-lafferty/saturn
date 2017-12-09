#pragma once

#include "../filesystem.h"
#include <vector>

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
        uint32_t firstNonReservedInode;
        uint16_t inodeSize;
        uint16_t blockGroup;
        uint32_t optionalFeatures;
        uint32_t requiredFeatures;
    };

    enum class RequiredFeatures {
        Compression = 0x1,
        DirectoryEntryTypeField = 0x2,
        ReplayJournalNeeded = 0x4,
        UseJournal = 0x8
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
        BlockGroupDescriptorTable,
        InodeTable,
        Inode,
        DirectoryEntry,
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
        uint32_t creationTime;
        uint32_t lastModificationTime;
        uint32_t deletionTime;
        uint16_t groupId;
        uint16_t hardLinkCount;
        uint32_t diskSectorsCount;
        uint32_t flags;
        uint32_t operatingSystem;
        uint32_t directBlock[12];
        uint32_t singlyIndirectBlock;
        uint32_t doublyIndirectBlock;
        uint32_t triplyIndirectBlock;
        uint32_t generationNumber;
        uint32_t extendedAttributes;
        uint32_t sizeUpper32Bits;
        uint32_t fragmentBlockId;
        uint8_t osSpecificValue[12];

        bool isDirectory();
    };

    enum class InodeType {
        FIFO = 0x1000,
        CharacterDevice = 0x2000,
        Directory = 0x4000,
        BlockDevice = 0x6000,
        RegularFile = 0x8000,
        SymbolicLink = 0xA000,
        UnixSocket = 0xC000
    };

    struct DirectoryEntry {
        uint32_t inode;
        uint16_t size;
        uint8_t nameLength;
        uint8_t typeIndicator;
    };

    class Ext2FileSystem : public FileSystem {
    public:
        Ext2FileSystem(IBlockDevice* device); 
        void receiveSector() override;

    private:

        uint32_t blockIdToLba(uint32_t blockId);

        void setupSuperBlock();
        void setupBlockDescriptorTable(uint16_t* buffer);
        void readInode(uint32_t id);
        void readDirectory(Inode& inode);

        SuperBlock superBlock;
        ReadState readState;
        uint32_t blockGroupCount;
        uint32_t blockSize;
        uint32_t blockGroupDescriptorTableId;
        /*
        cache the whole block group descriptor table here, its only
        max 8 megs

        TODO: whenever a block group descriptor changes, this
        needs to be updated
        */
        std::vector<BlockGroupDescriptor> blockGroupDescriptorTable;
        uint16_t* buffer;
    };
}