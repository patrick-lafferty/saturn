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
#include "features.h"
#include <string.h>
#include <system_calls.h>
#include <services/virtualFileSystem/virtualFileSystem.h>
#include <algorithm>

using namespace VirtualFileSystem;
using namespace Vostok;

namespace HardwareFileSystem {

    CPUFeaturesObject::CPUFeaturesObject() {
        
    }

    void CPUFeaturesObject::readSelf(uint32_t requesterTaskId, uint32_t requestId) {
        ReadResult result;
        result.success = true;
        result.requestId = requestId;
        ArgBuffer args {result.buffer, sizeof(result.buffer)};
        args.writeType(ArgTypes::Property);

        args.writeValueWithType("instruction_sets", ArgTypes::Cstring);
        args.writeValueWithType("instructions", ArgTypes::Cstring);
        args.writeValueWithType("support", ArgTypes::Cstring);
        args.writeValueWithType("extensions", ArgTypes::Cstring);
        args.writeValueWithType("technology", ArgTypes::Cstring);

        args.writeType(ArgTypes::EndArg);

        result.recipientId = requesterTaskId;
        send(IPC::RecipientType::TaskId, &result);
    }

    /*
    Vostok Object property support
    */
    int CPUFeaturesObject::getProperty(std::string_view) {
        return -1;
    }

    void CPUFeaturesObject::readProperty(uint32_t requesterTaskId, uint32_t requestId, uint32_t) {
        ReadResult result;
        result.success = false;
        result.requestId = requestId;
        result.recipientId = requesterTaskId;
        send(IPC::RecipientType::TaskId, &result);
    }

    void CPUFeaturesObject::writeProperty(uint32_t requesterTaskId, uint32_t requestId, uint32_t, ArgBuffer&) {
        replyWriteSucceeded(requesterTaskId, requestId, false);
    }

    Object* CPUFeaturesObject::getNestedObject(std::string_view name) {
        if (name.compare("instruction_sets") == 0) {
            return &instructionSets;
        }
        else if (name.compare("instructions") == 0) {
            return &instructions;
        }
        else if (name.compare("support") == 0) {
            return &support;
        }
        else if (name.compare("extensions") == 0) {
            return &extensions;
        }
        else if (name.compare("technology") == 0) {
            return &technology;
        }

        return nullptr;
    }

    /*
    CPU Features Object specific implementation
    */

    const char* CPUFeaturesObject::getName() {
        return "features";
    }

    void detectFeatures(uint32_t ecx, uint32_t edx) {

        detectInstructionSets(ecx);
        detectInstructions(ecx, edx);
        detectSupport(ecx, edx);
        detectExtensions(ecx, edx);
        detectTechnology(ecx);
    }
}