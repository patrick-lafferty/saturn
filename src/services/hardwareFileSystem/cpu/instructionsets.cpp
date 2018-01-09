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
#include "instructionsets.h"
#include <string.h>
#include <system_calls.h>
#include <services/virtualFileSystem/virtualFileSystem.h>
#include <algorithm>

using namespace VirtualFileSystem;
using namespace Vostok;

namespace HardwareFileSystem {

    CPUInstructionSetsObject::CPUInstructionSetsObject() {
        features = 0;
    }

    void CPUInstructionSetsObject::readSelf(uint32_t requesterTaskId, uint32_t requestId) {
        ReadResult result;
        result.success = true;
        result.requestId = requestId;
        ArgBuffer args {result.buffer, sizeof(result.buffer)};
        args.writeType(ArgTypes::Property);

        args.writeValueWithType("sse3", ArgTypes::Cstring);
        args.writeValueWithType("ssse3", ArgTypes::Cstring);
        args.writeValueWithType("sse4_1", ArgTypes::Cstring);
        args.writeValueWithType("sse4_2", ArgTypes::Cstring);
        args.writeValueWithType("avx", ArgTypes::Cstring);

        args.writeType(ArgTypes::EndArg);

        result.recipientId = requesterTaskId;
        send(IPC::RecipientType::TaskId, &result);
    }

    /*
    Vostok Object property support
    */
    int CPUInstructionSetsObject::getProperty(std::string_view name) {
        if (name.compare("sse3") == 0) {
            return static_cast<int>(PropertyId::SSE3);
        }
        else if (name.compare("ssse3") == 0) {
            return static_cast<int>(PropertyId::SSSE3);
        }
        else if (name.compare("sse4_1") == 0) {
            return static_cast<int>(PropertyId::SSE4_1);
        }
        else if (name.compare("sse4_2") == 0) {
            return static_cast<int>(PropertyId::SSE4_2);
        }
        else if (name.compare("avx") == 0) {
            return static_cast<int>(PropertyId::AVX);
        }

        return -1;
    }

    void CPUInstructionSetsObject::readProperty(uint32_t requesterTaskId, uint32_t requestId, uint32_t propertyId) {
        ReadResult result;
        result.success = true;
        result.requestId = requestId;
        ArgBuffer args {result.buffer, sizeof(result.buffer)};
        args.writeType(ArgTypes::Property);

        args.writeValueWithType((features & (1 << propertyId)) > 0, ArgTypes::Uint32);

        args.writeType(ArgTypes::EndArg);

        result.recipientId = requesterTaskId;
        send(IPC::RecipientType::TaskId, &result);
    }

    void CPUInstructionSetsObject::writeProperty(uint32_t requesterTaskId, uint32_t requestId, uint32_t propertyId, ArgBuffer& args) {
        auto type = args.readType();

        if (type != ArgTypes::Property) {
            replyWriteSucceeded(requesterTaskId, requestId, false);
            return;
        }

        auto x = args.read<uint32_t>(ArgTypes::Uint32);

        if (!args.hasErrors()) {
            if (x > 0) {
                features |= (1 << propertyId);
            }
            replyWriteSucceeded(requesterTaskId, requestId, true);
            return;
        }

        replyWriteSucceeded(requesterTaskId, requestId, false);
    }

    /*
    CPU Features Object specific implementation
    */

    const char* CPUInstructionSetsObject::getName() {
        return "instruction_sets";
    }

    void detectInstructionSets(uint32_t ecx) {
        auto sse3 = (ecx & (1u << 0));
        auto ssse3 = (ecx & (1u << 9));
        auto sse4_1 = (ecx & (1u << 19));
        auto sse4_2 = (ecx & (1u << 20));
        auto avx = (ecx & (1u << 28));

        writeTransaction("/system/hardware/cpu/features/instruction_sets/sse3", sse3, ArgTypes::Uint32);
        writeTransaction("/system/hardware/cpu/features/instruction_sets/ssse3", ssse3, ArgTypes::Uint32);
        writeTransaction("/system/hardware/cpu/features/instruction_sets/sse4_1", sse4_1, ArgTypes::Uint32);
        writeTransaction("/system/hardware/cpu/features/instruction_sets/sse4_2", sse4_2, ArgTypes::Uint32);
        writeTransaction("/system/hardware/cpu/features/instruction_sets/avx", avx, ArgTypes::Uint32);
    }
}