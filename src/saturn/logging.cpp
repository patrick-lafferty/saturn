/*
Copyright (c) 2018, Patrick Lafferty
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

#include "logging.h"
#include <system_calls.h>
#include <stdio.h>
#include <services/virtualFileSystem/vostok.h>

namespace Saturn::Log {

    Logger::Logger(std::string name)
        : name {name} {
        
        char path[256];
        memset(path, '\0', 256);
        sprintf(path, "/events/%s", name.data());

        create(path);

        auto openResult = openSynchronous(path);

        if (openResult.success) {
            fileDescriptor = openResult.fileDescriptor;
        }
    }

    void Logger::info(char* format, ...) {
        if (format == nullptr
            || !fileDescriptor) {
            return;
        }

        va_list args;
        va_start(args, format);

        char buffer[256];
        memset(buffer, '\0', 256);
        vsprintf(buffer, format, args);

        write(fileDescriptor.value(), buffer, strlen(buffer));

        va_end(args);
    }

    void subscribe() {
        auto openResult = openSynchronous("/events/subscribe");

        if (openResult.success) {
            auto readResult = readSynchronous(openResult.fileDescriptor, 0);
            Vostok::ArgBuffer args {readResult.buffer, sizeof(readResult.buffer)};
            args.readType();

            args.write(openResult.recipientId, Vostok::ArgTypes::Uint32);

            if (readResult.success) {
                write(openResult.fileDescriptor, readResult.buffer, sizeof(readResult.buffer));
            }
        }
    }
}

