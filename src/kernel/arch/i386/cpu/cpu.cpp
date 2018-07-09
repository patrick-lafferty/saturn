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
#include "msr.h"
#include "apic.h"

namespace CPU {

    int getCurrentCPUId() {
        //return readModelSpecificRegister_Low(ModelSpecificRegister::IA32_TSC_AUX);
        int eax, edx, ecx;
        asm volatile("rdtscp" : "=a" (eax), "=d" (edx), "=c" (ecx));
        return ecx;
    }

    void sendIPI(APIC::InterprocessorInterrupt ipi) {
        for (int i = 1; i < CPUCount; i++) {
            APIC::sendInterprocessorInterrupt(ActiveCPUs[i].apicId, ipi);
        }
    }

    void setupCPUBlocks(int numberOfCPUs, CPUControlBlock initialCPU) {
        Kernel::BlockAllocator<CPUControlBlock> allocator {Memory::getCurrentVMM()};
        ActiveCPUs = allocator.allocateMultiple(numberOfCPUs);
        ActiveCPUs[0] = initialCPU;
    }

    void exitCurrentTask() {
        auto cpuId = getCurrentCPUId();
        ActiveCPUs[cpuId].scheduler->exitTask();
    }

    void exitKernelTask() {
        auto cpuId = getCurrentCPUId();
        ActiveCPUs[cpuId].scheduler->exitKernelTask();
    }

    void sleepCurrentTask(uint32_t time) {
        auto cpuId = getCurrentCPUId();
        ActiveCPUs[cpuId].scheduler->blockTask(Kernel::BlockReason::Sleep, time);
    }

    void notifyTimesliceExpired() {
        auto cpuId = getCurrentCPUId();
        ActiveCPUs[cpuId].scheduler->notifyTimesliceExpired();
    }

    void setupTimeslice(bool propagate) {
        auto cpuId = getCurrentCPUId();

        if (propagate) {
            sendIPI(APIC::InterprocessorInterrupt::SetupTimeslice);
        }

        ActiveCPUs[cpuId].scheduler->setupTimeslice();
    }

    void sendMessage(IPC::RecipientType recipient, IPC::Message* message) {
        auto cpuId = getCurrentCPUId();
        ActiveCPUs[cpuId].scheduler->sendMessage(recipient, message);
    }

    void receiveMessage(IPC::Message* buffer) {
        auto cpuId = getCurrentCPUId();
        ActiveCPUs[cpuId].scheduler->receiveMessage(buffer);
    }

    void receiveMessage(IPC::Message* buffer, IPC::MessageNamespace filter, uint32_t messageId) {
        auto cpuId = getCurrentCPUId();
        ActiveCPUs[cpuId].scheduler->receiveMessage(buffer, filter, messageId);
    }

    bool peekReceiveMessage(IPC::Message* buffer) {
        auto cpuId = getCurrentCPUId();
        return ActiveCPUs[cpuId].scheduler->peekReceiveMessage(buffer);
    }

    void scheduleTask(Kernel::Task* task) {
        //TODO: find appropriate core to run task on
        static int index = 1;
        ActiveCPUs[index].scheduler->scheduleTask(task);

        if (index == 0) index = 1;
        else index = 0;
    }

    void startScheduler() {
        sendIPI(APIC::InterprocessorInterrupt::StartScheduler);
    }

    void invalidateTLB() {
        sendIPI(APIC::InterprocessorInterrupt::InvalidateTLB);
    }

    void reschedule() {
        auto cpuId = getCurrentCPUId();
        ActiveCPUs[cpuId].scheduler->reschedule();
    }

    Kernel::Task* getTask(uint32_t id) {
        return Kernel::TaskStore::getInstance().getTask(id);
    }

    void changePriority(Kernel::Task* task, Kernel::Priority priority) {
        auto cpuId = getCurrentCPUId();
        return ActiveCPUs[cpuId].scheduler->changePriority(task, priority);
    }
}
