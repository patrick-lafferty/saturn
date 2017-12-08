#pragma once

#include "../filesystem.h"

namespace MassStorageFileSystem::Ext2 {
    
    class Ext2FileSystem : public FileSystem {
    public:
        Ext2FileSystem(Partition partition); 

    private:
    };
}