#pragma once

#include <stdint.h>
#include <vector>

namespace VirtualFileSystem::Cache {

    enum class Type {
        File,
        Directory,
        Union
    };

    struct Entry {
        Type type;
        bool cacheable {true};
        bool needsRead {true};
        bool writeable {true};
        char path[256];
        uint32_t index;
    };

    /*
    Cache::Directory root;
    root's children: [process, system/hardware, bin]

    / [mount request says cacheable]
        process [mount request says not cacheable]
        system 
            hardware [mount request says not cacheable]
        bin [mount request says not cacheable]

    on mounting /, vfs reads / directory, receives:
        testA/, testB/, C

    */

    struct File : Entry {
        //data

    };

    struct Directory : Entry {

        Directory() {
            type = Type::Directory;
        }

        uint32_t mount;
        std::vector<Entry*> children;
    };

    struct Union : Entry {
        Union() {
            type = Type::Union;
        }

        std::vector<Directory*> children;
    };


    /*

    mounting hfs to /system/hardware:

    union /
        <- no children

    create a dummy system Directory thats 
        not cacheable
        not needsRead
        not writeable

    with a single child:

        create a dummy hardware Directory thats
            nont cacheable
            not needsRead
            not writeable
            mount = taskId

    */
}