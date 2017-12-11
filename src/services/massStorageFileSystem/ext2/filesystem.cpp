#include "filesystem.h"
#include <stdio.h>
#include <string.h>

namespace MassStorageFileSystem::Ext2 {

    bool Inode::isDirectory() {
        return typeAndPermissions & static_cast<uint16_t>(InodeType::Directory);
    }

    Ext2FileSystem::Ext2FileSystem(IBlockDevice* device) 
        : FileSystem(device) {
        
        //superblock spans 2 sectors, but I don't care about the second
        //sector contents... yet
        buffer = new uint16_t[256];
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

    void Ext2FileSystem::readDirectory(Inode& inode) {
        auto blocksToRead = inode.sizeLower32Bits / blockSize;
        
        for (int i = 0; i < blocksToRead; i++) {
            auto lba = blockIdToLba(inode.directBlock[i]);
            blockDevice->queueReadSector(lba, 1);
            break;
        }

        readState = ReadState::DirectoryEntry;
    }

    void Ext2FileSystem::receiveSector() {

        memset(buffer, 0, 512);
        blockDevice->receiveSector(buffer);

        switch(readState) {
            case ReadState::SuperBlock: {
                memcpy(&superBlock, buffer, sizeof(SuperBlock));
                setupSuperBlock();

                break;
            }
            case ReadState::BlockGroupDescriptorTable: {

                setupBlockDescriptorTable(buffer);
                readInode(2);
                break;
            }
            case ReadState::InodeTable: {
                auto inodes = reinterpret_cast<Inode*>(buffer);
                auto inode = inodes[(ino - 1) % superBlock.inodesPerGroup];

                if (inode.isDirectory()) {
                    readDirectory(inode);
                }

                break;
            }
            case ReadState::DirectoryEntry: {

                uint8_t* ptr = reinterpret_cast<uint8_t*>(buffer);
                auto entry = *reinterpret_cast<DirectoryEntry*>(ptr);

                for (int i = 0; i < 512; i++) {
                    ptr += sizeof(DirectoryEntry);
                    i += sizeof(DirectoryEntry);

                    if (entry.inode != 0 && entry.nameLength > 0) {

                        char name[256];

                        memcpy(name, ptr, entry.nameLength);
                        name[entry.nameLength] = '\0';

                        printf("[Ext2] found: %s\n", name);
                    }

                    ptr += entry.size - sizeof(DirectoryEntry);
                    i += entry.size - sizeof(DirectoryEntry);
                    entry = *reinterpret_cast<DirectoryEntry*>(ptr);
                }

                break;
            }
        }
    }

    void Ext2FileSystem::readDirectory(uint32_t index, uint32_t requestId) {
        /*
        Steps:

        1 - read the inode, 1 queueReadSector
        2 - read the blocks of the inode, x queueReadSectors
        3 - for each block, extract the directory entries, send response message
        */

    }
}