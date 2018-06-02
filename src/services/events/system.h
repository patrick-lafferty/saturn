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
#include <variant>
#include <optional>
#include <services/virtualFileSystem/virtualFileSystem.h>
#include <services/virtualFileSystem/vostok.h>

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
        std::variant<Log*, Vostok::FileDescriptor> object;
    };

    struct ReceiveSignature {
        VirtualFileSystem::ReadResult result;
        Vostok::ArgBuffer args;
    };

    class EventSystem : public Vostok::Object {
    public:

        void messageLoop();

        virtual void readSelf(uint32_t , uint32_t ) override {}
        virtual int getProperty(std::string_view ) override { return -1; }
        virtual void readProperty(uint32_t , uint32_t , uint32_t ) override {}
        virtual void writeProperty(uint32_t , uint32_t , uint32_t , Vostok::ArgBuffer& ) override {}
        virtual Object* getNestedObject(std::string_view ) override { return nullptr; }

        virtual int getFunction(std::string_view) override;
        virtual void readFunction(uint32_t requesterTaskId, uint32_t requestId, uint32_t functionId) override;
        virtual void writeFunction(uint32_t requesterTaskId, uint32_t requestId, uint32_t functionId, Vostok::ArgBuffer& args) override;
        virtual void describeFunction(uint32_t requesterTaskId, uint32_t requestId, uint32_t functionId) override;

    private:

        void handleOpenRequest(VirtualFileSystem::OpenRequest& request);
        void handleCreateRequest(VirtualFileSystem::CreateRequest& request);
        void handleReadRequest(VirtualFileSystem::ReadRequest& request);
        void handleWriteRequest(VirtualFileSystem::WriteRequest& request);
        void forwardToSerialPort(VirtualFileSystem::WriteRequest& request);

        std::vector<std::unique_ptr<Log>> logs;
        std::vector<FileDescriptor> openDescriptors;
        uint32_t nextDescriptorId {0};

        enum class FunctionId {
            Subscribe
        };

        void subscribe(uint32_t requesterTaskId, uint32_t requestId, uint32_t subscriberTaskId);
        void broadcastEvent(std::string_view event);

        std::vector<uint32_t> subscribers;
        std::optional<ReceiveSignature> receiveSignature;
        uint32_t serialFileDescriptor;
    };

    void service();
}