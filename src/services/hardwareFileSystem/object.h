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

#include <services/virtualFileSystem/vostok.h>
#include <system_calls.h>

namespace HardwareFileSystem {

    class HardwareObject : public Vostok::Object {
    public:

        virtual int getFunction(std::string_view) override;
        virtual void readFunction(uint32_t requesterTaskId, uint32_t requestId, uint32_t functionId) override;
        virtual void writeFunction(uint32_t requesterTaskId, uint32_t requestId, uint32_t functionId, Vostok::ArgBuffer& args) override;
        virtual void describeFunction(uint32_t requesterTaskId, uint32_t requestId, uint32_t functionId) override;

        virtual Object* getNestedObject(std::string_view name) override;

        virtual const char* getName() = 0;
    };

    void replyWriteSucceeded(uint32_t requesterTaskId, uint32_t requestId, bool success, bool expectReadResult = false);

    template<typename Arg>
    void writeTransaction(const char* path, Arg value, Vostok::ArgTypes argType) {
        auto openResult = openSynchronous(path);

        if (openResult.success) {
            auto readResult = readSynchronous(openResult.fileDescriptor, 0);

            if (readResult.success) {
                Vostok::ArgBuffer args{readResult.buffer, sizeof(readResult.buffer)};

                auto type = args.readType();

                if (type == Vostok::ArgTypes::Property) {
                    args.write(value, argType);
                }

                write(openResult.fileDescriptor, readResult.buffer, sizeof(readResult.buffer));
                receiveAndIgnore();
            }

            close(openResult.fileDescriptor);
            receiveAndIgnore();
        }
    }
}