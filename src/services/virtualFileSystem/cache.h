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

namespace VirtualFileSystem::Cache {

    enum class Type : uint8_t {
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
        uint32_t pathLength;
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