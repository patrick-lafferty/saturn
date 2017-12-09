#include "filesystem.h"
#include <stdio.h>
#include <string.h>

namespace MassStorageFileSystem::Ext2 {

    Ext2FileSystem::Ext2FileSystem(IBlockDevice* device) 
        : FileSystem(device) {
        
        //superblock spans 2 sectors, but I don't care about the second
        //sector contents... yet
        blockDevice->queueReadSector(2, 1);
        readState = ReadState::SuperBlock;
    }

    /*
    hacky invalid implementation, want to wait to just port clang's libc++
    instead of writing cmath
    */
    uint32_t ceil(double x) {
        if (x == (uint32_t)x) {
            return x;
        }
        else {
            return x + 1;
        }
    }

    uint32_t Ext2FileSystem::blockIdToLba(uint32_t blockId) {
        const uint32_t sectorSize = 512;

        return (blockId * blockSize) / sectorSize;
    }

    void Ext2FileSystem::setupSuperBlock() {
        auto blockGroups = ceil((double)superBlock.totalBlocks / superBlock.blocksPerGroup);
        auto check = ceil(superBlock.totalInodes / superBlock.inodesPerGroup);

        if (blockGroups != check) {
            printf("[Ext2] Invalid number of block groups detected: %d %d\n",
                blockGroups, check);
        }

        blockGroupCount = blockGroups;

        if (superBlock.log2BlockSize > 0) {
            blockGroupDescriptorTableId = 1;
        }
        else {
            blockGroupDescriptorTableId = 2;
        }

        blockSize = 1024 << superBlock.log2BlockSize;

        blockDevice->queueReadSector(blockIdToLba(blockGroupDescriptorTableId), 1);
        readState = ReadState::BlockGroupDescriptorTable;
    }

    void Ext2FileSystem::setupBlockDescriptorTable(uint16_t* buffer) {
        auto descriptors = reinterpret_cast<BlockGroupDescriptor*>(buffer);

        for (auto i = 0u; i < blockGroupCount; i++) {
            blockGroupDescriptorTable.push_back(*descriptors);
            descriptors++;
        }
    }

    uint32_t ino;

    void Ext2FileSystem::readInode(uint32_t id) {
        auto blockGroup = (id - 1) / superBlock.inodesPerGroup;
        auto& descriptor = blockGroupDescriptorTable[blockGroup];
        ino = id;

        auto inodeIndex = (id - 1) % superBlock.inodesPerGroup;
        auto blockId = descriptor.inodeTableId + (inodeIndex * superBlock.inodeSize) / blockSize;
        auto lba = blockIdToLba(blockId);
        blockDevice->queueReadSector(lba, 1);
        readState = ReadState::InodeTable;
    }

    void Ext2FileSystem::receiveSector() {
        uint16_t buffer[256];
        blockDevice->receiveSector(buffer);

        switch(readState) {
            case ReadState::SuperBlock: {
                memcpy(&superBlock, buffer, sizeof(superBlock));
                setupSuperBlock();

                break;
            }
            case ReadState::BlockGroupDescriptorTable: {

printf("[Ext2] reading blockGroupDescriptorTable %x\n", blockGroupCount);
                setupBlockDescriptorTable(buffer);
                readInode(2);
printf("[Ext2]  queued\n");
                break;
            }
            case ReadState::InodeTable: {
                auto inodes = reinterpret_cast<Inode*>(buffer);
                auto inode = inodes[(ino - 1) % superBlock.inodesPerGroup];

                printf("[Ext2] Inode type: %x\n", inode.typeAndPermissions);
            
                break;
            }
        }
    }
}