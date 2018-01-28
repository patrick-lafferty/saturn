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
#include "wait.h"
#include <stdint.h>
#include <system_calls.h>
#include "services.h"
#include <services/virtualFileSystem/messages.h>
#include <stdio.h>

namespace Saturn::Event {

    void waitForMount(std::string_view path) {
        VirtualFileSystem::SubscribeMount subscribe;
        subscribe.serviceType = Kernel::ServiceType::VFS;
        path.copy(subscribe.path, sizeof(subscribe.path));

        send(IPC::RecipientType::ServiceName, &subscribe);

        while (true) {
            IPC::MaximumMessageBuffer buffer;
            filteredReceive(&buffer, IPC::MessageNamespace::VFS, 
                static_cast<uint32_t>(VirtualFileSystem::MessageId::MountNotification));

            auto notify = IPC::extractMessage<VirtualFileSystem::MountNotification>(buffer);

            if (path.compare(notify.path) == 0) {
                return;
            }
            else {
                kprintf("[libSaturn] waitForMount notified about an unexpected path\n");
            }
        }
    }
}