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

#include <stdint.h>
#include <vector>
#include <queue>
#include <list>
#include "filesystem.h"
#include <services/virtualFileSystem/messages.h>

namespace ATA {
    class Driver;
}

namespace MassStorageFileSystem {
    void service();

    uint64_t convert64ToLittleEndian(uint64_t input);
    uint32_t convert32ToLittleEndian(uint32_t input);
    uint16_t convert16ToLittleEndian(uint16_t input);

    struct GPTHeader {
        uint8_t signature[8];
        uint32_t revision;
        uint32_t headerSize;
        uint32_t headerCRC32;
        uint32_t reserved;
        uint64_t currentLBA;
        uint64_t backupLBA;
        uint64_t firstUsableLBA;
        uint64_t lastUsableLBA;
        GUID diskGUID; 
        uint64_t partitionArrayLBA;
        uint32_t partitionEntriesCount;
        uint32_t partitionEntrySize;
        uint32_t partitionArrayCRC;
        uint8_t remaining[164];

        uint8_t extra[256];
    };

    enum class PendingDiskCommand {
        Read,
        Write,
        None
    };

    struct Request {
        uint32_t lba;
        uint32_t sectorCount;
        uint32_t requesterId;
    };

    class MassStorageController {
    public:

        MassStorageController(ATA::Driver* driver);
        void preloop();
        void messageLoop();

    private:

        void queueReadSectorRequest(uint32_t lba, uint32_t sectorCount, uint32_t requesterId);
        void queueReadSector(Request request);

        void handleGetDirectoryEntries(VirtualFileSystem::GetDirectoryEntries& request);
        void handleReadRequest(VirtualFileSystem::ReadRequest& request);
        void handleSeekRequest(VirtualFileSystem::SeekRequest& request);

        ATA::Driver* driver;
        GPTHeader gptHeader;
        PendingDiskCommand pendingCommand {PendingDiskCommand::None};
        std::vector<Partition> partitions;
        std::vector<FileSystem*> fileSystems;
        std::queue<Request, std::list<Request>> queuedRequests;
    };
}