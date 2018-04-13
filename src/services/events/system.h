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

#include <vector>
#include <string>
#include <memory>
#include <services/virtualFileSystem/virtualFileSystem.h>

namespace Event {

    struct Log {
        
        Log(std::string_view logName) {
            logName.copy(name, logName.length());
        }

        void addEntry(std::string_view entry);

        char name[256];
        std::vector<std::string> entries;
    };

    struct FileDescriptor {
        uint32_t id;
        Log* log;
    };

    class EventSystem {
    public:

        void messageLoop();

    private:

        void handleOpenRequest(VirtualFileSystem::OpenRequest& request);
        void handleCreateRequest(VirtualFileSystem::CreateRequest& request);
        void handleWriteRequest(VirtualFileSystem::WriteRequest& request);

        std::vector<std::unique_ptr<Log>> logs;
        std::vector<FileDescriptor> openDescriptors;
        uint32_t nextDescriptorId {0};
    };

    void service();
}