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

namespace ATA {

    /*
    Some registers behave differently when
    reading vs writing, indicated with comments below
    */
    enum class Register {
        Data,
        /*
        Reading Features gives "Error",
        */
        Features,
        SectorCount,
        LBALow,
        LBAMid,
        LBAHigh,
        Device,
        /*
        Reading Command gives "Status"
        */
        Command,
        /*
        Reading Control gives "Alternate" status
        */
        Control = 0x206 //3f6 - 1f0
    };

    enum class Status {
        Error = 1 << 0,
        SenseDataAvailable = 1 << 1,
        AlignmentError = 1 << 2,
        DataRequest = 1 << 3,
        DeferredWriteError = 1 << 4,
        DeviceFault = 1 << 5,
        DeviceReady = 1 < 6,
        Busy = 1 << 7
    };


    enum class Device {
        Master,
        Slave,
        None
    };

    enum class Command {
        Reset = 0x08,
        ReadSectors = 0x20,
        ReadMultiple = 0xC4,
        Identify = 0xEC
    };

    struct IdentifyDeviceData {
        uint16_t generalConfig;
        uint16_t obsolete0;
        uint16_t specific0;
        uint16_t obsolete1;
        uint16_t retired0[2];
        uint16_t obsolete2;
        uint16_t reservedForCFA[2];
        uint16_t retired1;
        uint16_t serialNumber[10];
        uint16_t retired2[2];
        uint16_t obsolete3;
        uint16_t firmwareRevision[4];
        uint16_t modelNumber[20];
        uint16_t something0;
        uint16_t trustedComputingFeatureSetOptions;
        uint16_t capabilities0;
        uint16_t capabilities1;
        uint16_t obsolete4[2];
        uint16_t something1;
        uint16_t obsolete5[5];
        uint16_t something2;
        uint16_t totalAddressable28bitSectors[2];
        uint16_t obsolete6;
        uint16_t something3;
        uint16_t something4;
        uint16_t minimumMultiwordDMATransferTime;
        uint16_t manufacturerRecommendedDMATransferTime;
        uint16_t minimumPIOTransferNoFlow;
        uint16_t minimumPIOTransferIORDY;
        uint16_t additionalSupported;
        uint16_t reserved0;
        uint16_t reservedForIdentifyPacket[4];
        uint16_t queueDepth;
        uint16_t sataCapabilities;
        uint16_t sataAdditionalCapabilities;
        uint16_t sataFeaturesSupported;
        uint16_t sataFeaturesEnabled;
        uint16_t majorVersionNumber;
        uint16_t minorVersionNumber;
        uint16_t commandsSupported;
        uint16_t commandsSupported2;
        uint16_t commandsSupported3;
        uint16_t commandsSupported4;
        uint16_t commandsSupported5;
        uint16_t commandsSupported6;
        uint16_t ultraDMAModes;
        uint16_t something5;
        uint16_t something6;
        uint16_t something7;
        uint16_t masterPasswordIdentifier;
        uint16_t hardwareResetResults;
        uint16_t obsolete7;
        uint16_t streamMinimumRequestSize;
        uint16_t streamTransferTimeDMA;
        uint16_t streamAccessLatencyDMAPIO;
        uint16_t streamPerformanceGranularity[2];
        uint16_t userAddressableLogicalSectors[4];
        uint16_t streamTransferTimePIO;
        uint16_t max512ByteBlocksPerDSM;
        uint16_t physLogSectorSize;
        uint16_t interseekDelay;
        uint16_t worldWideName[4];
        uint16_t reserved1[4];
        uint16_t obsolete8;
        uint16_t logicalSectorSize[2];
        uint16_t commandsSupported7;
        uint16_t commandsSupported8;
        uint16_t reserved2[5];
        uint16_t obsolete9;
        uint16_t securityStatus;
        uint16_t vendorSpecific[31];
        uint16_t reservedForCFA2[8];
        uint16_t something8;
        uint16_t dsmSupport;
        uint16_t additionalProductIdentifier[4];
        uint16_t reserved3[2];
        uint16_t currentMediaSerialNumber[30];
        uint16_t sctCommandTransport;
        uint16_t reserved4[2];
        uint16_t alignment;
        uint16_t writeReadVerifySectorMode3Count[2];
        uint16_t writeReadVerifySectorMode2Count[2];
        uint16_t obsolete10[3];
        uint16_t nominalMediaRotationRate;
        uint16_t reserved5;
        uint16_t obsolete11;
        uint16_t something9;
        uint16_t reserved6;
        uint16_t transportMajorVersionNumber;
        uint16_t transportMinorVersionNumber;
        uint16_t reserved7[6];
        uint16_t extendedUserAddressableSectors[4];
        uint16_t minimumBlocksPerMicrocodeDownload;
        uint16_t maximumBlocksPerMicrocodeDownload;
        uint16_t reserved8[19];
        uint16_t integritryWord;
    };

    class Driver {
    public: 

        Driver(uint8_t device, uint8_t function);

        void queueReadSector(uint32_t lba, uint32_t sectorCount);
        bool receiveSector(uint16_t* buffer);
        
    private:

        void selectDevice(Device device);
        void resetDevice(Device device);

        Device currentDevice {Device::None}; 
    };
}