/*
Copyright (c) 2017, Patrick Lafferty
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

    * Redistributions of source code must retain the above copyright
      notice, this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright
      notice, this list of conditions and the following disclaimer in the
      documentation and/or other materials provided with the distribution.
    * Neither the name of the copyright holder nor the names of its 
      contributors may be used to endorse or promote products derived from 
      this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR 
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/
#pragma once

#include "../filesystem.h"
#include <vector>
#include <queue>
#include <list>

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
        uint16_t pad;
        uint8_t reserved[12];
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

    enum class State {
        ReadingSuperBlock,
        ReadingBlockGroupDescriptorTable,

    };

    /*
    Requests that FileSystems get:

    1) directory contents for a path (represented by a uint32)
    2) file contents for a path (represented by a uint32)
    
    directory contents: given an int (inode), return a struct of directory entires
    -read the inode
    -

    */

    enum class ReadProgress {
        Inode,
        DirectBlock,
        IndirectBlockList,
        SinglyIndirectBlockListCache,
        DoublyIndirectBlockList,
        IndirectBlock
    };

    enum class CacheStatus {
        NotCaching,
        ReadSinglyIndirectLower,
        ReadSinglyIndirectHigher,
        ReadDoublyIndirectLower,
        ReadDoublyIndirectHigher,
    };

    struct ReadRequest {
        uint32_t inode;
        uint32_t remainingBlocks;
        uint32_t remainingBytes;
        bool finishedReadingInode;
        bool finishedReadingBlocks;
        uint32_t requestId;
        uint32_t totalRemainingSectors;
        uint32_t remainingSectorsInBlock;
        ReadProgress state;
        uint32_t currentBlockId;
        uint32_t indirectSectorsRemaining;
        CacheStatus cacheStatus {CacheStatus::NotCaching};
    };

    enum class RequestType {
        ReadSuperblock,
        ReadBlockGroupDescriptorTable,
        ReadDirectory,
        ReadFile,
        ReadInode,
        SyncPositionWithCache
    };

    struct Request {
        ReadRequest read;
        RequestType type;

        uint32_t descriptor {0};
        uint32_t length {0};
        uint32_t position {0};
    };

    struct CachedBlock {
        uint8_t data[1024];
        bool hasLowerHalf {false};
        bool hasHigherHalf {false};
        int cacheHits {0};
    };

    struct FileDescriptor {
        Inode inode;
        uint32_t id;
        uint32_t filePosition;
        uint32_t requestId;
        uint32_t length;

        CachedBlock singlyIndirectCache;
        CachedBlock doublyIndirectCache;
    };

    struct RequestMeta {
        uint32_t startingBlock;
        uint32_t startingPosition;
        uint32_t remainingSectors;
        uint32_t blocksToRead;
    };

    class Ext2FileSystem : public FileSystem {
    public:
        Ext2FileSystem(IBlockDevice* device); 
        bool receiveSector() override;
        uint32_t openFile(uint32_t index, uint32_t requestId) override;
        void readDirectory(uint32_t index, uint32_t requestId) override;
        void readFile(uint32_t index, uint32_t requestId, uint32_t byteCount, uint32_t position) override;
        void seekFile(uint32_t index, uint32_t requestId, uint32_t offset, Origin origin) override;
        void syncPositionWithCache(uint32_t index, uint32_t position) override;

    private:

        uint32_t blockIdToLba(uint32_t blockId);

        void setupSuperBlock();
        void setupBlockDescriptorTable(uint16_t* buffer);
        void readInode(uint32_t id);
        uint32_t readInodeBlocks(Inode& inode);

        RequestMeta prepareFileReadRequest(FileDescriptor* descriptor, uint32_t length);
        void handleRequest(Request& request);    
        void finishRequest();
        void handleReadDirectoryRequest(ReadRequest& request);
        void handleReadFileRequest(Request& request);
        void handleReadInodeRequest(Request& request);

        void readSinglyIndirectList(FileDescriptor* descriptor, Request& request, uint32_t* blockIds);
        void afterFileReadBlock(Request& request);

        SuperBlock superBlock;
        //ReadState readState;
        uint32_t blockGroupCount;
        uint32_t blockSize;
        uint32_t blockGroupDescriptorTableId;
        uint32_t inodesPerBlock;
        uint32_t sectorsPerBlock;
        uint32_t sectorSize;
        /*
        cache the whole block group descriptor table here, its only
        max 8 megs

        TODO: whenever a block group descriptor changes, this
        needs to be updated
        */
        std::vector<BlockGroupDescriptor> blockGroupDescriptorTable;
        uint16_t* buffer;
        std::queue<Request, std::list<Request>> queuedRequests;
        std::vector<FileDescriptor> openFileDescriptors;
    };
}