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
        auto sectorsToRead = ceil((double)inode.sizeLower32Bits / sectorSize);
        auto totalSectors = sectorsToRead;

        #ifdef VERBOSE_DEBUG
        printf("[ATA] readInodeBlocks: blocks: %d, sectors: %d\n", blocksToRead, sectorsToRead);
        #endif
        
        for (auto i = 0u; i < blocksToRead; i++) {
            auto lba = blockIdToLba(inode.directBlock[i]);
            auto sectorCount = std::min(sectorsPerBlock, sectorsToRead);
            sectorsToRead -= sectorCount;
            blockDevice->queueReadSector(lba, sectorCount);
        }

        return totalSectors;
    }

    FileDescriptor* findDescriptor(uint32_t id, std::vector<FileDescriptor>& descriptors) {
        for (auto& descriptor : descriptors) {
            if (descriptor.id == id) {
                return &descriptor;
            }
        }

        return nullptr;
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
            case RequestType::SyncPositionWithCache: {
                auto descriptor = findDescriptor(request.descriptor, openFileDescriptors);

                if (descriptor != nullptr) {
                    descriptor->filePosition = request.position;
                }

                //finishRequest();

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

            request.totalRemainingSectors = readInodeBlocks(inode);
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
            request.totalRemainingSectors--;

            result.expectMore = request.totalRemainingSectors > 0;
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

            result.expectMore = request.totalRemainingSectors > 0;
            send(IPC::RecipientType::ServiceName, &result);

            if (request.totalRemainingSectors == 0) {
                finishRequest();
            }
        }
    }

    void Ext2FileSystem::readSinglyIndirectList(FileDescriptor* descriptor, Request& request, uint32_t* blockIds) {
        auto meta = prepareFileReadRequest(descriptor, request.read.remainingBytes);
        auto startingId = meta.startingBlock - 12;

        if (meta.startingBlock > 268) {
            startingId = (meta.startingBlock - 268) % 256;

            if (startingId >= 128) {
                startingId -= 128;
            }

            meta.blocksToRead = std::min(255u, meta.blocksToRead);
        }

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

            if (meta.remainingSectors == 0) {
                break;
            }
        }

        request.read.indirectSectorsRemaining--;

        if (request.read.indirectSectorsRemaining == 0) {
            request.read.state = ReadProgress::IndirectBlock;
        }
    }

    void Ext2FileSystem::handleReadFileRequest(Request& request) {
        auto descriptor = findDescriptor(request.descriptor, openFileDescriptors);

        if (descriptor == nullptr) {
            finishRequest();
            return;
        }

        if (0 != request.position) {
            descriptor->filePosition = request.position;
        }
        
        if (!request.read.finishedReadingBlocks) {
            request.read.finishedReadingBlocks = true;

            auto meta = prepareFileReadRequest(descriptor, request.length);
            
            request.read.totalRemainingSectors = meta.remainingSectors;
            request.read.remainingBlocks = meta.blocksToRead;
            request.read.remainingSectorsInBlock = sectorsPerBlock - (descriptor->filePosition % blockSize) / sectorSize;
            request.read.remainingBytes = request.length;

            for (auto i = 0u; i < meta.blocksToRead; i++) {
                auto blockId = meta.startingBlock + i;
                auto remainingSectorsInBlock = sectorsPerBlock - (meta.startingPosition % blockSize) / sectorSize;               
                auto sectorCount = std::min(remainingSectorsInBlock, meta.remainingSectors);
                meta.remainingSectors -= sectorCount;
                meta.startingPosition = ((meta.startingPosition / blockSize) + 1) * blockSize;

                if (blockId < 12) {
                    auto lba = blockIdToLba(descriptor->inode.directBlock[blockId]);
                    lba += (sectorsPerBlock - remainingSectorsInBlock); 
                    blockDevice->queueReadSector(lba, sectorCount);
                }
                else if (blockId > 11 && blockId < 266) {
                    auto lba = blockIdToLba(descriptor->inode.singlyIndirectBlock);
                    auto indirectSectors = 2;
                    blockId -= 12;

                    if (blockId > 128) {
                        lba++;
                        indirectSectors = 1;

                        if (!descriptor->singlyIndirectCache.hasHigherHalf) {
                            request.read.cacheStatus = CacheStatus::ReadSinglyIndirectHigher;
                        }
                    }
                    else if ((blockId + sectorCount / sectorsPerBlock) < 128) {
                        indirectSectors = 1;

                        if (!descriptor->singlyIndirectCache.hasLowerHalf) {
                            request.read.cacheStatus = CacheStatus::ReadSinglyIndirectLower;
                        }
                    }
                    else {
                        auto pause = 0;
                    }

                    request.read.indirectSectorsRemaining = indirectSectors;
                    request.read.totalRemainingSectors += indirectSectors;
                    request.read.remainingSectorsInBlock = indirectSectors;

                    if (request.read.cacheStatus == CacheStatus::NotCaching) {
                        uint32_t* blockIds = reinterpret_cast<uint32_t*>(descriptor->singlyIndirectCache.data);

                        if (blockId > 128) {
                            blockIds += 128;
                        }

                        descriptor->singlyIndirectCache.cacheHits++;

                        readSinglyIndirectList(descriptor, request, blockIds);
                        afterFileReadBlock(request);
                        return;
                    }
                    else {
                        blockDevice->queueReadSector(lba, indirectSectors);
                    }

                    break;
                }
                else if (blockId > 268 && blockId < 65804) {
                    blockId -= 268;
                    blockId /= 256;
                    auto lba = blockIdToLba(descriptor->inode.doublyIndirectBlock);
                    auto indirectSectors = 2;

                    if (blockId > 255) {
                        lba++;
                        indirectSectors = 1;
                    }

                    auto endBlock = (descriptor->filePosition + request.length) / blockSize;
                    endBlock = (endBlock - 268) / 256;

                    if (blockId < 256 && endBlock > 255) {
                        indirectSectors = 2;
                    }
                    else {
                        indirectSectors = 1;
                    }

                    blockDevice->queueReadSector(lba, indirectSectors);//sectorsPerBlock);
                    request.read.indirectSectorsRemaining = indirectSectors;//sectorsPerBlock;
                    request.read.totalRemainingSectors += indirectSectors; //sectorsPerBlock;
                    request.read.remainingSectorsInBlock = indirectSectors;
                    break;
                }
                else {
                    printf("[Ext2] Stub: Ext2FileSystem::handleReadRequest doesn't support triple indirect blocks yet\n");
                    asm("hlt");
                    break;
                }

            }
        
            if (meta.startingBlock < 12) {
                request.read.state = ReadProgress::DirectBlock;
            }
            else if (meta.startingBlock < 266 && request.read.cacheStatus != CacheStatus::NotCaching) {
                request.read.state = ReadProgress::IndirectBlockList;
            }
            else {
                request.read.state = ReadProgress::DoublyIndirectBlockList;
            }

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

                    if (request.read.cacheStatus == CacheStatus::ReadSinglyIndirectLower) {
                        memcpy(descriptor->singlyIndirectCache.data,
                            buffer, 
                            sectorSize);
                        descriptor->singlyIndirectCache.hasLowerHalf = true;
                    }
                    else if (request.read.cacheStatus == CacheStatus::ReadSinglyIndirectHigher) {
                        memcpy(descriptor->singlyIndirectCache.data + sectorSize,
                            buffer, 
                            sectorSize);
                        descriptor->singlyIndirectCache.hasHigherHalf = true;
                    }

                    auto blockIds = reinterpret_cast<uint32_t*>(buffer);
                    readSinglyIndirectList(descriptor, request, blockIds); 

                    break;
                }
                case ReadProgress::DoublyIndirectBlockList: {
                    auto blockIds = reinterpret_cast<uint32_t*>(buffer);
                    auto meta = prepareFileReadRequest(descriptor, request.read.remainingBytes);
                    auto startingId = meta.startingBlock - 268;
                    auto totalIndirectSectors = 0;

                    for (auto i = 0u; i < meta.blocksToRead; i++) {
                        auto blockId = (startingId + i) / 256;
                        auto id = *(blockIds + blockId);

                        if (id == 0) {
                            break;
                        }

                        //auto remainingSectorsInBlock = sectorsPerBlock - (meta.startingPosition % blockSize) / sectorSize;
                        auto sectorCount = std::min(sectorsPerBlock, meta.remainingSectors); 
                        auto lba = blockIdToLba(id);

                        bool skip = false;    
                        if (((startingId + i) % 256) >= 128) {
                            lba++;
                            sectorCount = 1;
                            skip = true;
                        }

                        blockDevice->queueReadSector(lba, sectorCount);
                        meta.remainingSectors -= sectorCount;

                        request.read.remainingBlocks++;
                        meta.startingPosition = ((meta.startingPosition / blockSize) + 1) * blockSize;
                        totalIndirectSectors += sectorCount;
                        request.read.totalRemainingSectors += sectorCount;

                        if (meta.remainingSectors == 0 || skip) {
                            break;
                        }
                    }

                    request.read.indirectSectorsRemaining--;

                    if (request.read.indirectSectorsRemaining == 0) {
                        request.read.state = ReadProgress::IndirectBlockList;
                        request.read.indirectSectorsRemaining = totalIndirectSectors;
                    }

                    break;
                }
                default: {
                    printf("[Ext2] Unhandled read state\n");
                }
            }

            afterFileReadBlock(request);
        }
    }

    void Ext2FileSystem::afterFileReadBlock(Request& request) {

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

    void Ext2FileSystem::handleReadInodeRequest(Request& request) {

        #ifdef VERBOSE_DEBUG
        printf("[ATA] handleReadInodeRequest\n");
        #endif

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

                    VirtualFileSystem::OpenResult result;
                    result.success = true;
                    result.serviceType = Kernel::ServiceType::VFS;
                    result.fileDescriptor = request.descriptor;
                    result.requestId = descriptor.requestId;
                    result.fileLength = descriptor.length;
                    send(IPC::RecipientType::ServiceName, &result);

                    finishRequest();
                    return;
                }
            }
        }
    }

    bool Ext2FileSystem::receiveSector() {

        #ifdef VERBOSE_DEBUG
        printf("[ATA] receiveSector\n");
        #endif

        memset(buffer, 0, 512);
        if (!blockDevice->receiveSector(buffer)) {
            printf("[ATA] receiveSector: blockDevice->receiveSector returned false\n");
            return false;
        }

        if (queuedRequests.empty()) {
            printf("[ATA] receiveSector: queuedRequests is empty\n");
            return false;
        }

        handleRequest(queuedRequests.front());

        return true;
    }

    uint32_t Ext2FileSystem::openFile(uint32_t index, uint32_t requestId) {
        static uint32_t nextFileDescriptor = 0;

        #ifdef VERBOSE_DEBUG
        printf("[ATA] openFile\n");
        #endif

        FileDescriptor descriptor;
        descriptor.filePosition = 0;
        descriptor.requestId = requestId;
        descriptor.id = nextFileDescriptor++;
        memset(descriptor.singlyIndirectCache.data, 0, blockSize);
        memset(descriptor.doublyIndirectCache.data, 0, blockSize);

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
        
        #ifdef VERBOSE_DEBUG
        printf("[ATA] readDirectory\n");
        #endif

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

    void Ext2FileSystem::readFile(uint32_t index, uint32_t requestId, uint32_t byteCount, uint32_t position) {

        #ifdef VERBOSE_DEBUG
        printf("[ATA] readFile\n");
        #endif

        Request request{};
        request.read.requestId = requestId;
        request.type = RequestType::ReadFile;
        request.descriptor = index;
        request.length = byteCount;
        request.position = position;

        if (queuedRequests.empty()) {
            handleRequest(request);
        }

        queuedRequests.push(request);
    }

    void Ext2FileSystem::seekFile(uint32_t index, uint32_t requestId, uint32_t offset, Origin origin) {

        #ifdef VERBOSE_DEBUG        
        printf("[ATA] seekFile\n");
        #endif

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

    void Ext2FileSystem::syncPositionWithCache(uint32_t index, uint32_t position) {
        Request request;
        request.type = RequestType::SyncPositionWithCache;
        request.descriptor = index;
        request.position = position;        

        //if (queuedRequests.empty()) {
            handleRequest(request);
        //}

        //queuedRequests.push(request);
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