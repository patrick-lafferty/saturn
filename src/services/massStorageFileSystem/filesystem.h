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
#include <utility>
#include <type_traits>

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
        virtual bool receiveSector(uint16_t* buffer) = 0;

    protected:

        Partition partition;
    };

    template<typename Read, typename Write>
    struct Requester {

        Requester(Read&& r, Write&& w) : read{std::forward<Read>(r)}, write{std::forward<Write>(w)} {}

        Read read;
        Write write; 
    };

    template<typename Read, typename Write>
    Requester<std::remove_reference_t<Read>, std::remove_reference_t<Write>> makeRequester(Read&& r, Write&& w) {
        return Requester<std::remove_reference_t<Read>, std::remove_reference_t<Write>>(std::forward<Read>(r), std::forward<Write>(w));
    }

    template<typename Read, typename Write>
    struct Transfer {

        Transfer(Read&& r, Write&& w) : read{std::forward<Read>(r)}, write{std::forward<Write>(w)} {}

        Read read;
        Write write; 
    };

    template<typename Read, typename Write>
    Transfer<std::remove_reference_t<Read>, std::remove_reference_t<Write>> makeTransfer(Read&& r, Write&& w) {
        return Transfer<std::remove_reference_t<Read>, std::remove_reference_t<Write>>(std::forward<Read>(r), std::forward<Write>(w));
    }

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

        virtual bool receiveSector(uint16_t* buffer) override {
            return transfer.read(buffer);
        }
    
    private:

        Requester requester;
        Transfer transfer;
    };

    enum class Origin {
        Current = 1,
        End = 2,
        Beginning = 3,
    };

    class FileSystem {
    public: 

        FileSystem(IBlockDevice* device) : blockDevice{device} {}
        virtual ~FileSystem() {}

        virtual bool receiveSector() = 0;
        virtual uint32_t openFile(uint32_t index, uint32_t requestId) = 0;
        virtual void readDirectory(uint32_t index, uint32_t requestId) = 0;
        virtual void readFile(uint32_t index, uint32_t requestId, uint32_t byteCount) = 0;
        virtual void seekFile(uint32_t index, uint32_t requestId, uint32_t offset, Origin origin) = 0;
        virtual void syncPositionWithCache(uint32_t index, uint32_t position) = 0;

    protected: 
        IBlockDevice* blockDevice;
    };
}