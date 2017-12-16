#include "filesystem.h"
#include <stdio.h>
#include <string.h>
#include <services/virtualFileSystem/messages.h>
#include <services.h>
#include <system_calls.h>

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
        queuedRequests.push({{}, RequestType::ReadSuperblock});
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
        memcpy(&superBlock, buffer, sizeof(SuperBlock));
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
        queuedRequests.pop();
        queuedRequests.push({{}, RequestType::ReadBlockGroupDescriptorTable});
    }

    void Ext2FileSystem::setupBlockDescriptorTable(uint16_t* buffer) {
        auto descriptors = reinterpret_cast<BlockGroupDescriptor*>(buffer);

        for (auto i = 0u; i < blockGroupCount; i++) {
            blockGroupDescriptorTable.push_back(*descriptors);
            descriptors++;
        }

        queuedRequests.pop();
    }

    void Ext2FileSystem::readInode(uint32_t id) {
        auto blockGroup = (id - 1) / superBlock.inodesPerGroup;
        auto& descriptor = blockGroupDescriptorTable[blockGroup];

        auto inodeIndex = (id - 1) % superBlock.inodesPerGroup;
        auto blockId = descriptor.inodeTableId + (inodeIndex * superBlock.inodeSize) / blockSize;
        auto lba = blockIdToLba(blockId);
        blockDevice->queueReadSector(lba, 1);
    }

    uint32_t Ext2FileSystem::readDirectory(Inode& inode) {
        auto blocksToRead = 1 + inode.sizeLower32Bits / blockSize;
        
        for (auto i = 0u; i < blocksToRead; i++) {
            auto lba = blockIdToLba(inode.directBlock[i]);
            blockDevice->queueReadSector(lba, 1);
        }

        return blocksToRead;
    }

    void Ext2FileSystem::handleRequest(Request& request) {
        switch(request.type) {
            case RequestType::ReadSuperblock: {
                setupSuperBlock();
                break;
            }
            case RequestType::ReadBlockGroupDescriptorTable: {
                setupBlockDescriptorTable(buffer);
                break;
            }
            case RequestType::ReadDirectory: {
                handleReadDirectoryRequest(request.read);
                break;
            }
        }
    }

    void Ext2FileSystem::finishRequest() {
        queuedRequests.pop();

        if (!queuedRequests.empty()) {
            handleRequest(queuedRequests.front());
        }
    }

    void Ext2FileSystem::handleReadDirectoryRequest(ReadRequest& request) {
        if (!request.finishedReadingInode) {
            readInode(request.inode);
            request.finishedReadingInode = true;
        }
        else if (!request.finishedReadingBlocks) {
            auto inodes = reinterpret_cast<Inode*>(buffer);
            auto inode = inodes[(request.inode - 1) % superBlock.inodesPerGroup];

            request.remainingBlocks = readDirectory(inode);
            request.finishedReadingBlocks = true;
        }
        else {
            uint8_t* ptr = reinterpret_cast<uint8_t*>(buffer);
            auto entry = *reinterpret_cast<DirectoryEntry*>(ptr);

            VirtualFileSystem::GetDirectoryEntriesResult result;
            result.requestId = request.requestId;
            result.serviceType = Kernel::ServiceType::VFS;
            memset(result.data, 0, sizeof(result.data));
            uint32_t remainingSpace = sizeof(result.data);
            uint32_t writeIndex = 0;
            bool needToSend;
            request.remainingBlocks--;

            result.expectMore = request.remainingBlocks > 0;

            for (int i = 0; i < 512; i++) {
                ptr += sizeof(DirectoryEntry);
                i += sizeof(DirectoryEntry);

                if (entry.inode != 0 && entry.nameLength > 0) {

                    //5 = (uint32_t index, uint8_t type)                    
                    if (remainingSpace < (entry.nameLength + 5 + 1)) {
                        result.expectMore = true;
                        send(IPC::RecipientType::ServiceName, &result);
                        writeIndex = 0;
                        remainingSpace = sizeof(result.data);
                        needToSend = false;
                    }

                    //map ext2's type indicator to vfs' type indicator
                    switch(entry.typeIndicator) {
                        case 1: {
                            entry.typeIndicator = 0;
                            break;
                        }
                        case 2: {
                            entry.typeIndicator = 1;
                            break;
                        }
                        default: {
                            entry.typeIndicator = 0;
                            break;
                        }
                    }

                    memcpy(result.data + writeIndex, &entry.inode, sizeof(entry.inode));
                    writeIndex += 4;
                    memcpy(result.data + writeIndex, &entry.typeIndicator, sizeof(entry.typeIndicator));
                    writeIndex += 1;
                    memcpy(result.data + writeIndex, ptr, entry.nameLength);
                    result.data[writeIndex + entry.nameLength] = '\0'; 
                    writeIndex += entry.nameLength + 1;
                    remainingSpace -= (5 + entry.nameLength + 1);
                    needToSend = true;
                }

                ptr += entry.size - sizeof(DirectoryEntry);
                i += entry.size - sizeof(DirectoryEntry);
                entry = *reinterpret_cast<DirectoryEntry*>(ptr);
            }

            if (needToSend) {
                result.expectMore = request.remainingBlocks > 0;
                send(IPC::RecipientType::ServiceName, &result);
            }
            else if (result.expectMore) {
                //nothing more to send, but we said to expect more, so send one more saying done
                result.expectMore = request.remainingBlocks > 0;
                send(IPC::RecipientType::ServiceName, &result);
            }

            if (request.remainingBlocks == 0) {
                finishRequest();
            }
        }
    }

    void Ext2FileSystem::receiveSector() {

        memset(buffer, 0, 512);
        blockDevice->receiveSector(buffer);

        if (queuedRequests.empty()) {
            return;
        }

        handleRequest(queuedRequests.front());
    }

    void Ext2FileSystem::readDirectory(uint32_t index, uint32_t requestId) {
        /*
        Steps:

        1 - read the inode, 1 queueReadSector
        2 - read the blocks of the inode, x queueReadSectors
        3 - for each block, extract the directory entries, send response message
        */
        Request request{};
        request.read.inode = index;
        
        if (index == 0) {
            request.read.inode = 2;
        }

        request.read.requestId = requestId;
        request.type = RequestType::ReadDirectory;

        if (queuedRequests.empty()) {
            handleRequest(request);
        }

        queuedRequests.push(request);
    }
}