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
#include "extensions.h"
#include <string.h>
#include <system_calls.h>
#include <services/virtualFileSystem/virtualFileSystem.h>
#include <algorithm>

using namespace VirtualFileSystem;
using namespace Vostok;

namespace HardwareFileSystem {

    CPUExtensionsObject::CPUExtensionsObject() {
        extensions = 0;        
    }

    void CPUExtensionsObject::readSelf(uint32_t requesterTaskId, uint32_t requestId) {
        ReadResult result;
        result.success = true;
        result.requestId = requestId;
        ArgBuffer args {result.buffer, sizeof(result.buffer)};
        args.writeType(ArgTypes::Property);

        args.writeValueWithType("vmx", ArgTypes::Cstring);
        args.writeValueWithType("smx", ArgTypes::Cstring);
        args.writeValueWithType("fma", ArgTypes::Cstring);
        args.writeValueWithType("fpu", ArgTypes::Cstring);
        args.writeValueWithType("vme", ArgTypes::Cstring);
        args.writeValueWithType("de", ArgTypes::Cstring);
        args.writeValueWithType("pse", ArgTypes::Cstring);
        args.writeValueWithType("pae", ArgTypes::Cstring);
        args.writeValueWithType("mce", ArgTypes::Cstring);
        args.writeValueWithType("pse_36", ArgTypes::Cstring);

        args.writeType(ArgTypes::EndArg);

        result.recipientId = requesterTaskId;
        send(IPC::RecipientType::TaskId, &result);
    }

    /*
    Vostok Object property support
    */
    int CPUExtensionsObject::getProperty(std::string_view name) {
        if (name.compare("vmx") == 0) {
            return static_cast<int>(PropertyId::VMX);
        }
        else if (name.compare("smx") == 0) {
            return static_cast<int>(PropertyId::SMX);
        }
        else if (name.compare("fma") == 0) {
            return static_cast<int>(PropertyId::FMA);
        }
        else if (name.compare("fpu") == 0) {
            return static_cast<int>(PropertyId::FPU);
        }
        else if (name.compare("vme") == 0) {
            return static_cast<int>(PropertyId::VME);
        }
        else if (name.compare("de") == 0) {
            return static_cast<int>(PropertyId::DE);
        }
        else if (name.compare("pse") == 0) {
            return static_cast<int>(PropertyId::PSE);
        }
        else if (name.compare("pae") == 0) {
            return static_cast<int>(PropertyId::PAE);
        }
        else if (name.compare("mce") == 0) {
            return static_cast<int>(PropertyId::MCE);
        }
        else if (name.compare("pse_36") == 0) {
            return static_cast<int>(PropertyId::PSE_36);
        }

        return -1;
    }

    void CPUExtensionsObject::readProperty(uint32_t requesterTaskId, uint32_t requestId, uint32_t propertyId) {
        ReadResult result;
        result.success = true;
        result.requestId = requestId;
        ArgBuffer args {result.buffer, sizeof(result.buffer)};
        args.writeType(ArgTypes::Property);

        args.writeValueWithType((extensions & (1 << propertyId)) > 0, ArgTypes::Uint32);

        args.writeType(ArgTypes::EndArg);

        result.recipientId = requesterTaskId;
        send(IPC::RecipientType::TaskId, &result);
    }

    void CPUExtensionsObject::writeProperty(uint32_t requesterTaskId, uint32_t requestId, uint32_t propertyId, ArgBuffer& args) {
        auto type = args.readType();

        if (type != ArgTypes::Property) {
            replyWriteSucceeded(requesterTaskId, requestId, false);
            return;
        }

        auto x = args.read<uint32_t>(ArgTypes::Uint32);

        if (!args.hasErrors()) {
            if (x > 0) {
                extensions |= (1 << propertyId);
            }
            replyWriteSucceeded(requesterTaskId, requestId, true);
            return;
        }

        replyWriteSucceeded(requesterTaskId, requestId, false);
    }

    /*
    CPU Features Object specific implementation
    */

    const char* CPUExtensionsObject::getName() {
        return "extensions";
    }

    void detectExtensions(uint32_t ecx, uint32_t edx) {

        auto vmx = (ecx & (1u << 5));
        auto smx = (ecx & (1u << 6));
        auto fma = (ecx & (1u << 12));
        auto fpu = (edx & (1u << 0));
        auto vme = (edx & (1u << 1));
        auto de = (edx & (1u << 2));
        auto pse = (edx & (1u << 3));
        auto pae = (edx & (1u << 6));
        auto mce = (edx & (1u << 7));
        auto pse_36 = (edx & (1u << 17));

        writeTransaction("/system/hardware/cpu/features/extensions/vmx", vmx, ArgTypes::Uint32);
        writeTransaction("/system/hardware/cpu/features/extensions/smx", smx, ArgTypes::Uint32);
        writeTransaction("/system/hardware/cpu/features/extensions/fma", fma, ArgTypes::Uint32);
        writeTransaction("/system/hardware/cpu/features/extensions/fpu", fpu, ArgTypes::Uint32);
        writeTransaction("/system/hardware/cpu/features/extensions/vme", vme, ArgTypes::Uint32);
        writeTransaction("/system/hardware/cpu/features/extensions/de", de, ArgTypes::Uint32);
        writeTransaction("/system/hardware/cpu/features/extensions/pse", pse, ArgTypes::Uint32);
        writeTransaction("/system/hardware/cpu/features/extensions/pae", pae, ArgTypes::Uint32);
        writeTransaction("/system/hardware/cpu/features/extensions/mce", mce, ArgTypes::Uint32);
        writeTransaction("/system/hardware/cpu/features/extensions/pse_36", pse_36, ArgTypes::Uint32);
    }
}