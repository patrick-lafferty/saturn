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
#include "timer.h"
#include <string.h>
#include <system_calls.h>
#include <services/virtualFileSystem/virtualFileSystem.h>
#include <algorithm>
#include <string.h>
#include <stdlib.h>
#include "tsc.h"

using namespace VirtualFileSystem;
using namespace Vostok;

namespace HardwareFileSystem::Timer {

    TimerObject::TimerObject() {

    }

    void TimerObject::readSelf(uint32_t requesterTaskId, uint32_t requestId) {
        ReadResult result;
        result.success = true;
        result.requestId = requestId;
        ArgBuffer args {result.buffer, sizeof(result.buffer)};
        args.writeType(ArgTypes::Property);

        args.writeType(ArgTypes::EndArg);

        result.recipientId = requesterTaskId;
        send(IPC::RecipientType::TaskId, &result);
    }

    /*
    Vostok Object property support
    */
    int TimerObject::getProperty(std::string_view) {
        return -1;
    }

    void TimerObject::readProperty(uint32_t requesterTaskId, uint32_t requestId, uint32_t) {
        ReadResult result;
        result.success = true;
        result.requestId = requestId;
        ArgBuffer args {result.buffer, sizeof(result.buffer)};
        args.writeType(ArgTypes::Property);

        args.writeType(ArgTypes::EndArg);

        result.recipientId = requesterTaskId;
        send(IPC::RecipientType::TaskId, &result);
    }

    void TimerObject::writeProperty(uint32_t requesterTaskId, uint32_t requestId, uint32_t, ArgBuffer&) {
        replyWriteSucceeded(requesterTaskId, requestId, false);
    }

    ::Object* TimerObject::getNestedObject(std::string_view name) {

        if (name.compare("tsc") == 0) {
            return &tsc;
        }

        return nullptr;
    }

    /*
    TimerObject specific implementation
    */

    const char* TimerObject::getName() {
        return "timer";
    }

    bool createTimerObject() {
        auto path = "/system/hardware/timer";
        create(path);
        
        IPC::MaximumMessageBuffer buffer;
        filteredReceive(&buffer, IPC::MessageNamespace::VFS, static_cast<uint32_t>(MessageId::CreateResult));

        auto result = IPC::extractMessage<CreateResult>(buffer);

        return result.success;
    }
}