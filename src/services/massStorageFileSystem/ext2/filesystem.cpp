#include "filesystem.h"
#include <stdio.h>
#include <string.h>
#include <services/virtualFileSystem/messages.h>
#include <services.h>
#include <system_calls.h>
#include <algorithm>

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
        sectorSize = 512;
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
        inodesPerBlock = blockSize / superBlock.inodeSize;
        sectorsPerBlock = blockSize / 512;

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

        if ((inodeIndex % inodesPerBlock) > 3) {
            lba++;
        }

        blockDevice->queueReadSector(lba, 1);
    }

    uint32_t Ext2FileSystem::readInodeBlocks(Inode& inode) {
        auto blocksToRead = ceil((double)inode.sizeLower32Bits / blockSize);
        auto sectorsToRead = ceil((double)inode.sizeLower32Bits / sectorSize);//1 + request.length / sectorSize;
        
        for (auto i = 0u; i < blocksToRead; i++) {
            auto lba = blockIdToLba(inode.directBlock[i]);
            auto sectorCount = std::min(sectorsPerBlock, sectorsToRead);
            sectorsToRead -= sectorCount;
            blockDevice->queueReadSector(lba, sectorCount);
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
            case RequestType::ReadFile: {
                handleReadFileRequest(request);
                break;
            }
            case RequestType::ReadInode: {
                handleReadInodeRequest(request);
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
            auto inodeIndex = (request.inode - 1) % inodesPerBlock;

            if (inodeIndex > 3) {
                inodeIndex -= 4;
            }
            
            auto inode = inodes[inodeIndex];

            request.remainingBlocks = readInodeBlocks(inode);
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
            auto entrySize = static_cast<uint32_t>(sizeof(DirectoryEntry));

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

                auto skipBytes = entry.size > entrySize ? (entry.size - entrySize) : entrySize;

                ptr += skipBytes;
                i += skipBytes;
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

    FileDescriptor* findDescriptor(uint32_t id, std::vector<FileDescriptor>& descriptors) {
        for (auto& descriptor : descriptors) {
            if (descriptor.id == id) {
                return &descriptor;
            }
        }

        return nullptr;
    }

    void Ext2FileSystem::handleReadFileRequest(Request& request) {
        auto descriptor = findDescriptor(request.descriptor, openFileDescriptors);

        if (descriptor == nullptr) {
            finishRequest();
            return;
        }
        
        if (!request.read.finishedReadingBlocks) {
            request.read.finishedReadingBlocks = true;

            auto meta = prepareFileReadRequest(descriptor, request.length);
            
            request.read.totalRemainingSectors = meta.remainingSectors;
            request.read.remainingBlocks = meta.blocksToRead;
            request.read.remainingSectorsInBlock = sectorsPerBlock - (descriptor->filePosition % blockSize) / sectorSize;

            for (auto i = 0u; i < meta.blocksToRead; i++) {
                auto blockId = meta.startingBlock + i;
                auto remainingSectorsInBlock = sectorsPerBlock - (meta.startingPosition % blockSize) / sectorSize;               
                auto sectorCount = std::min(remainingSectorsInBlock, meta.remainingSectors);
                meta.remainingSectors -= sectorCount;

                if (blockId < 12) {
                    auto lba = blockIdToLba(descriptor->inode.directBlock[blockId]);
                    lba += (sectorsPerBlock - remainingSectorsInBlock); 
                    blockDevice->queueReadSector(lba, sectorCount);
                }
                else if (blockId > 11 && blockId < 266) {
                    auto lba = blockIdToLba(descriptor->inode.singlyIndirectBlock);
                    blockDevice->queueReadSector(lba, sectorsPerBlock);
                    request.read.indirectSectorsRemaining = sectorsPerBlock;
                    request.read.totalRemainingSectors += sectorsPerBlock;
                    request.read.remainingSectorsInBlock = 2;
                    break;
                }
                else {
                    printf("[Ext2] Stub: Ext2FileSystem::handleReadRequest doesn't support double/triple indirect blocks yet\n");
                    break;
                }

                meta.startingPosition = ((meta.startingPosition / blockSize) + 1) * blockSize;
            }
        
            if (meta.startingBlock < 12) {
                request.read.state = ReadProgress::DirectBlock;
            }
            else {
                request.read.state = ReadProgress::IndirectBlockList;
            }

            request.read.remainingBytes = request.length;
        }
        else {

            switch (request.read.state) {
                case ReadProgress::DirectBlock:
                case ReadProgress::IndirectBlock: {
                    VirtualFileSystem::Read512Result result;
                    result.requestId = request.read.requestId;
                    result.serviceType = Kernel::ServiceType::VFS;
                    result.success = true;

                    auto offset = descriptor->filePosition % sectorSize;
                    auto count = std::min(static_cast<uint32_t>(sizeof(result.buffer))  - offset, request.read.remainingBytes);
                    auto buff = reinterpret_cast<uint8_t*>(buffer);
                    memcpy(result.buffer, buff + offset, count);
                    result.bytesWritten = count;
                    result.expectMore = request.read.totalRemainingSectors > 1;
                    descriptor->filePosition += count; 
                    request.read.remainingBytes -= count;
                    send(IPC::RecipientType::ServiceName, &result);
                    break;
                }
                case ReadProgress::IndirectBlockList: {

                    auto blockIds = reinterpret_cast<uint32_t*>(buffer);
                    auto meta = prepareFileReadRequest(descriptor, request.read.remainingBytes);
                    auto startingId = meta.startingBlock - 12;

                    for (auto i = 0u; i < meta.blocksToRead; i++) {
                        auto blockId = startingId + i;
                        auto id = *(blockIds + blockId);

                        if (id == 0) {
                            break;
                        }

                        auto remainingSectorsInBlock = sectorsPerBlock - (meta.startingPosition % blockSize) / sectorSize;
                        auto sectorCount = std::min(sectorsPerBlock, meta.remainingSectors); 
                        auto lba = blockIdToLba(id);

                        lba += (sectorsPerBlock - remainingSectorsInBlock);
                        blockDevice->queueReadSector(lba, sectorCount);
                        meta.remainingSectors -= sectorCount;

                        request.read.remainingBlocks++;
                        meta.startingPosition = ((meta.startingPosition / blockSize) + 1) * blockSize;
                    }

                    request.read.indirectSectorsRemaining--;

                    if (request.read.indirectSectorsRemaining == 0) {
                        request.read.state = ReadProgress::IndirectBlock;
                    }

                    break;
                }
            }

            request.read.totalRemainingSectors--;
            request.read.remainingSectorsInBlock--;

            if (request.read.remainingSectorsInBlock == 0) {
                request.read.currentBlockId++;
                request.read.remainingSectorsInBlock = std::min(sectorsPerBlock, request.read.totalRemainingSectors);

                if (request.read.state == ReadProgress::DirectBlock
                    && request.read.currentBlockId == 12) {
                    request.read.state = ReadProgress::IndirectBlockList;
                }
                //todo: doubly indirect block list
            }      

            if (request.read.totalRemainingSectors == 0) {
                finishRequest();
            }
        }
    }

    void Ext2FileSystem::handleReadInodeRequest(Request& request) {

        if (!request.read.finishedReadingInode) {
            readInode(request.read.inode);
            request.read.finishedReadingInode = true;
        }
        else {
            auto inodes = reinterpret_cast<Inode*>(buffer);
            auto inodeIndex = (request.read.inode - 1) % inodesPerBlock;

            if (inodeIndex > 3) {
                inodeIndex -= 4;
            }
            
            auto& inode = inodes[inodeIndex];

            for (auto& descriptor : openFileDescriptors) {
                if (descriptor.id == request.descriptor) {
                    descriptor.inode = inode;
                    descriptor.length = inode.sizeLower32Bits;

                    finishRequest();
                    return;
                }
            }
        }
    }

    bool Ext2FileSystem::receiveSector() {

        memset(buffer, 0, 512);
        if (!blockDevice->receiveSector(buffer)) {
            return false;
        }

        if (queuedRequests.empty()) {
            return false;
        }

        handleRequest(queuedRequests.front());

        return true;
    }

    uint32_t Ext2FileSystem::openFile(uint32_t index, uint32_t requestId) {
        static uint32_t nextFileDescriptor = 0;

        FileDescriptor descriptor;
        descriptor.filePosition = 0;
        descriptor.requestId = requestId;
        descriptor.id = nextFileDescriptor++;

        openFileDescriptors.push_back(descriptor);

        Request request{};
        request.read.inode = index;
        
        if (index == 0) {
            request.read.inode = 2;
        }

        request.type = RequestType::ReadInode;
        request.descriptor = descriptor.id;

        if (queuedRequests.empty()) {
            handleRequest(request);
        }

        queuedRequests.push(request);

        return descriptor.id;
    }

    void Ext2FileSystem::readDirectory(uint32_t index, uint32_t requestId) {
        
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

    void Ext2FileSystem::readFile(uint32_t index, uint32_t requestId, uint32_t byteCount) {
        
        Request request{};
        request.read.requestId = requestId;
        request.type = RequestType::ReadFile;
        request.descriptor = index;
        request.length = byteCount;

        if (queuedRequests.empty()) {
            handleRequest(request);
        }

        queuedRequests.push(request);
    }

    void Ext2FileSystem::seekFile(uint32_t index, uint32_t requestId, uint32_t offset, Origin origin) {
        auto descriptor = findDescriptor(index, openFileDescriptors);
        VirtualFileSystem::SeekResult result;
        result.requestId = requestId;
        result.serviceType = Kernel::ServiceType::VFS;
        result.success = false;

        if (descriptor != nullptr) {
            switch(origin) {
                case Origin::Current: {
                    descriptor->filePosition += offset;
                    break;
                }
                case Origin::End: {
                    descriptor->filePosition = descriptor->length - offset;
                    break;
                }
                case Origin::Beginning: {
                    descriptor->filePosition = offset;
                    break;
                }
            }

            result.filePosition = descriptor->filePosition;
            result.success = true;
        }

        send(IPC::RecipientType::ServiceName, &result);
    }

    RequestMeta Ext2FileSystem::prepareFileReadRequest(FileDescriptor* descriptor, uint32_t length) {
        RequestMeta meta;

        meta.startingBlock = descriptor->filePosition / blockSize;
        auto endPosition = descriptor->filePosition + length;
        auto endingBlock = endPosition / blockSize;

        meta.blocksToRead = 1 + (endingBlock - meta.startingBlock);

        if (endingBlock != meta.startingBlock
            && ((endPosition % blockSize) == 0)) {
            meta.blocksToRead--;
        }

        auto startingSector = descriptor->filePosition / sectorSize;
        auto endingSector = endPosition / sectorSize;
        meta.remainingSectors = 1 + (endingSector - startingSector);

        if (endingSector != startingSector
            && (endPosition % sectorSize) == 0) {
            meta.remainingSectors--;
        }

        meta.startingPosition = descriptor->filePosition;

        return meta;
    }
}