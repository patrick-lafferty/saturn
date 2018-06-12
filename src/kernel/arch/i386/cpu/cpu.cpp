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
#include "cpu.h"
#include <memory/block_allocator.h>
#include <memory/virtual_memory_manager.h>
#include <scheduler.h>
#include <task.h>

namespace CPU {

    uint32_t TSS_ADDRESS = 0xcfff'f000;

    void setupCPUBlocks(int numberOfCPUs, CPUControlBlock initialCPU) {
        Kernel::BlockAllocator<CPUControlBlock> allocator {Memory::currentVMM};
        ActiveCPUs = allocator.allocateMultiple(numberOfCPUs);
        ActiveCPUs[0] = initialCPU;
    }

    void exitCurrentTask() {
        auto cpuId = *reinterpret_cast<uint32_t*>(TSS_ADDRESS);
        ActiveCPUs[cpuId].scheduler->exitTask();
    }

    void exitKernelTask() {
        auto cpuId = *reinterpret_cast<uint32_t*>(TSS_ADDRESS);
        ActiveCPUs[cpuId].scheduler->exitKernelTask();
    }

    void sleepCurrentTask(uint32_t time) {
        auto cpuId = *reinterpret_cast<uint32_t*>(TSS_ADDRESS);
        ActiveCPUs[cpuId].scheduler->blockTask(Kernel::BlockReason::Sleep, time);
    }

    void notifyTimesliceExpired() {
        auto cpuId = *reinterpret_cast<uint32_t*>(TSS_ADDRESS);
        ActiveCPUs[cpuId].scheduler->notifyTimesliceExpired();
    }

    void setupTimeslice() {
        auto cpuId = *reinterpret_cast<uint32_t*>(TSS_ADDRESS);
        ActiveCPUs[cpuId].scheduler->setupTimeslice();
    }

    void sendMessage(IPC::RecipientType recipient, IPC::Message* message) {
        auto cpuId = *reinterpret_cast<uint32_t*>(TSS_ADDRESS);
        ActiveCPUs[cpuId].scheduler->sendMessage(recipient, message);
    }

    void receiveMessage(IPC::Message* buffer) {
        auto cpuId = *reinterpret_cast<uint32_t*>(TSS_ADDRESS);
        ActiveCPUs[cpuId].scheduler->receiveMessage(buffer);
    }

    void receiveMessage(IPC::Message* buffer, IPC::MessageNamespace filter, uint32_t messageId) {
        auto cpuId = *reinterpret_cast<uint32_t*>(TSS_ADDRESS);
        ActiveCPUs[cpuId].scheduler->receiveMessage(buffer, filter, messageId);
    }

    bool peekReceiveMessage(IPC::Message* buffer) {
        auto cpuId = *reinterpret_cast<uint32_t*>(TSS_ADDRESS);
        return ActiveCPUs[cpuId].scheduler->peekReceiveMessage(buffer);
    }

    void scheduleTask(Kernel::Task* task) {
        //TODO: find appropriate core to run task on
        ActiveCPUs[0].scheduler->scheduleTask(task);
    }

    Kernel::Task* getTask(uint32_t id) {
        return Kernel::TaskStore::getInstance().getTask(id);
    }

    void changePriority(Kernel::Task* task, Kernel::Priority priority) {
        auto cpuId = *reinterpret_cast<uint32_t*>(TSS_ADDRESS);
        return ActiveCPUs[cpuId].scheduler->changePriority(task, priority);
    }
}
