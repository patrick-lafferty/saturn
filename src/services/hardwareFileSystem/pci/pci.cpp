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
#include "pci.h"
#include <string.h>
#include <system_calls.h>
#include <services/virtualFileSystem/virtualFileSystem.h>
#include <algorithm>

using namespace VirtualFileSystem;
using namespace Vostok;

namespace HardwareFileSystem::PCI {

    Object::Object() {

    }

    void Object::readSelf(uint32_t requesterTaskId, uint32_t requestId) {
        ReadResult result;
        result.success = true;
        result.requestId = requestId;
        ArgBuffer args {result.buffer, sizeof(result.buffer)};
        args.writeType(ArgTypes::Property);

        args.writeValueWithType("host0", ArgTypes::Cstring);

        args.writeType(ArgTypes::EndArg);

        result.recipientId = requesterTaskId;
        send(IPC::RecipientType::TaskId, &result);
    }

    /*
    Vostok Object function support
    */
    int Object::getFunction(std::string_view name) {
        if (name.compare("find") == 0) {
            return static_cast<int>(FunctionId::Find);
        }

        return -1;
    }

    void Object::readFunction(uint32_t requesterTaskId, uint32_t requestId, uint32_t functionId) {
        describeFunction(requesterTaskId, requestId, functionId);
    }

    void Object::writeFunction(uint32_t requesterTaskId, uint32_t requestId, uint32_t functionId, ArgBuffer& args) {
        auto type = args.readType();

        if (type != ArgTypes::Function) {
            replyWriteSucceeded(requesterTaskId, requestId, false);
            return;
        }

        switch(functionId) {
            case static_cast<uint32_t>(FunctionId::Find): {

                auto classCode = args.read<uint32_t>(ArgTypes::Uint32);
                auto subclassCode = args.read<uint32_t>(ArgTypes::Uint32);

                if (!args.hasErrors()) {
                    find(requesterTaskId, requestId, classCode, subclassCode);
                }
                else {
                    replyWriteSucceeded(requesterTaskId, requestId, false);
                }

                break;
            }
            default: {
                replyWriteSucceeded(requesterTaskId, requestId, false);
            }
        }
    }

    void Object::describeFunction(uint32_t requesterTaskId, uint32_t requestId, uint32_t functionId) {
        ReadResult result {};
        result.requestId = requestId;
        result.success = true;
        ArgBuffer args{result.buffer, sizeof(result.buffer)};
        args.writeType(ArgTypes::Function);
        
        switch(functionId) {
            case static_cast<uint32_t>(FunctionId::Find): {
                args.writeType(ArgTypes::Uint32);
                args.writeType(ArgTypes::Uint32);
                //TODO: need to add ArgTypes::ReturnType to allow for multiple return values
                //for now, we can encode 8-bit bus, 5-bit device and 3-bit function 
                //in one 32bit word
                args.writeType(ArgTypes::Uint32);
                args.writeType(ArgTypes::EndArg);
                break;
            }
            default: {
                result.success = false;
            }
        }

        result.recipientId = requesterTaskId;
        send(IPC::RecipientType::TaskId, &result);
    }

    /*
    Vostok Object property support
    */
    int Object::getProperty(std::string_view) {
        return -1;
    }

    void Object::readProperty(uint32_t requesterTaskId, uint32_t requestId, uint32_t) {
        ReadResult result;
        result.success = true;
        result.requestId = requestId;
        ArgBuffer args {result.buffer, sizeof(result.buffer)};
        args.writeType(ArgTypes::Property);

        args.writeType(ArgTypes::EndArg);

        result.recipientId = requesterTaskId;
        send(IPC::RecipientType::TaskId, &result);
    }

    void Object::writeProperty(uint32_t requesterTaskId, uint32_t requestId, uint32_t, ArgBuffer&) {
        replyWriteSucceeded(requesterTaskId, requestId, false);
    }

    Vostok::Object* Object::getNestedObject(std::string_view name) {
        if (name.compare("host0") == 0) {
            return &host0;
        } 

        return nullptr;
    }

    /*
    PCIObject specific implementation
    */

    const char* Object::getName() {
        return "pci";
    }

    void Object::find(uint32_t requesterTaskId, uint32_t requestId, uint32_t classCode, uint32_t subclassCode) {
        replyWriteSucceeded(requesterTaskId, requestId, true, true);

        ReadResult result;
        result.success = true;
        result.requestId = requestId;
        ArgBuffer args {result.buffer, sizeof(result.buffer)};
        args.writeType(ArgTypes::Property);

        uint32_t combinedIds {0};
        bool done{false};

        for (int deviceId = 0; deviceId < 32; deviceId++) {
            char id[3];
            sprintf(id, "%d", deviceId);
            id[2] = '\0';

            auto device = host0.getNestedObject(id);
            id[1] = '\0';

            for (int functionId = 0; functionId < 8; functionId++) {
                sprintf(id, "%d", functionId);

                auto function = static_cast<FunctionObject*>(device->getNestedObject(id));

                if (function->getClassCode() == classCode
                    && function->getSubClassCode() == subclassCode) {
                    //for now, hardcode bus 0
                    combinedIds = (0 << 16) | (deviceId << 8) | (functionId);
                    done = true;
                    break;
                }
            }

            if (done) {
                break;
            }
        }

        args.writeValueWithType(combinedIds, ArgTypes::Uint32);

        args.writeType(ArgTypes::EndArg);

        result.recipientId = requesterTaskId;
        send(IPC::RecipientType::TaskId, &result);
    }
}