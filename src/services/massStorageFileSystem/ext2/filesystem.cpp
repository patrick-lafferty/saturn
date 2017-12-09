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

    void Ext2FileSystem::receiveSector() {
        uint16_t buffer[256];
        blockDevice->receiveSector(buffer);

        switch(readState) {
            case ReadState::SuperBlock: {
                memcpy(&superBlock, buffer, sizeof(superBlock));
                break;
            }
        }
    }
}