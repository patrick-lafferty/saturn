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
#include "scheduler.h"
#include <stdio.h>
#include <memory/physical_memory_manager.h>
#include <memory/virtual_memory_manager.h>
#include <cpu/apic.h>
#include <cpu/cpu.h>
#include <cpu/msr.h>
#include <saturn/heap.h>
#include "ipc.h"
#include <new>
#include <services.h>
#include <stdlib.h>
#include <system_calls.h>
#include "task.h"
#include <locks.h>

extern "C" void changeProcess(Kernel::Task* current, Kernel::Task* next);
extern "C" void changeProcessSingle(Kernel::Task* current, Kernel::Task* next);
extern "C" void idleLoop();

namespace Kernel {

    void cleanupTasksService() {
        //currentScheduler->cleanupTasks();
    }

    void schedulerService() {
        while (true) {
            IPC::MaximumMessageBuffer buffer;
            receive(&buffer);

            switch (buffer.messageNamespace) {
                case IPC::MessageNamespace::Scheduler: {
                    switch (static_cast<MessageId>(buffer.messageId)) {
                        case MessageId::RunProgram: {

                            auto run = IPC::extractMessage<RunProgram>(buffer);
                            char* path = nullptr;

                            if (run.path[0] != '\0') {
                                path = run.path;
                            }

                            auto task = CurrentTaskLauncher->createUserTask(run.entryPoint);//, path);
                            task->priority = run.priority;
                            CPU::scheduleTask(task);

                            RunResult result;
                            result.recipientId = run.senderTaskId;
                            result.success = task != nullptr;
                            result.pid = task->id;
                            send(IPC::RecipientType::TaskId, &result);

                            break;
                        }
                        default: {
                            kprintf("[Scheduler] Unhandled message\n");
                        }
                    }

                    break;
                }
                default: {
                    kprintf("[Scheduler] Unhandled message namespace\n");
                }

                break;
            }
        }
    }

    Scheduler::Scheduler() {
        startTask = CurrentTaskLauncher->createKernelTask(reinterpret_cast<uintptr_t>(idleLoop));
        startTask->priority = Priority::Idle;
        scheduleTask(startTask);
        currentTask = nullptr;
        /*cleanupTask = createKernelTask(reinterpret_cast<uint32_t>(cleanupTasksService));
        cleanupTask->state = TaskState::Blocked;*/
        schedulerTask = CurrentTaskLauncher->createKernelTask(reinterpret_cast<uintptr_t>(schedulerService));
        schedulerTask->state = TaskState::Blocked;
        //blockedQueue.append(cleanupTask);
        blockedQueue.append(schedulerTask);

        elapsedTime_milliseconds = 0;
        timeslice_milliseconds = 10;
    }

    /*
    void Scheduler::cleanupTasks() {
        while (true) {
            asm volatile("cli");
            auto oldHeap = LibC_Implementation::KernelHeap;
            LibC_Implementation::KernelHeap = kernelHeap;
            auto oldVMM = Memory::getCurrentVMM();
            kernelVMM->activate();
            auto task = deleteQueue.getHead();

            while (task != nullptr) {
                delete task->kernelStack; 
                auto next = task->nextTask;
                deleteQueue.remove(task);
                task = next;
            }

            oldVMM->activate();
            LibC_Implementation::KernelHeap = oldHeap;

            asm volatile("sti");
            blockTask(BlockReason::WaitingForMessage, 0);
        }
    }*/

    void Scheduler::scheduleNextTask() {
        
        auto task = findNextTask();

        if (task == nullptr) {
            unblockWakeableTasks();
            task = findNextTask();
        }

        if (task == nullptr) {
            nextTask = startTask;
        }
        else {
            nextTask = task;
        }

        priorityGroups[static_cast<int>(nextTask->priority)].remove(nextTask);
        scheduleTask(nextTask);
    }

    Task* Scheduler::findNextTask() {
        
        for (int i = 0; i < static_cast<int>(Priority::Idle); i++) {
            auto& queue = priorityGroups[i];

            if (!queue.isEmpty()) {
                return queue.getHead();
            }
        }

        return nullptr;
    }

    void Scheduler::runNextTask() {
        if (currentTask == nextTask) {
            return;
        }

        nextTask->cpuId = CPU::getCurrentCPUId();

        if (CPU::ActiveCPUs != nullptr) {
            CPU::ActiveCPUs[nextTask->cpuId].heap = nextTask->heap;
        }

        auto oldTSS = reinterpret_cast<uint32_t>(nextTask->tss);
        auto newTSS = 0xCFFF'F000 - 0x1000 * nextTask->cpuId;
        auto oldVMM = Memory::getCurrentVMM();
        nextTask->virtualMemoryManager->activate();
        nextTask->virtualMemoryManager->remap(oldTSS, newTSS);
        nextTask->tss = reinterpret_cast<CPU::TSS*>(newTSS);
        oldVMM->activate();

        if (currentTask == nullptr) {
            currentTask = nextTask;
            changeProcessSingle(nullptr, nextTask);
        }
        else {

            auto current = currentTask;
            currentTask = nextTask;

            changeProcess(current, nextTask);
        }
    }

    void Scheduler::notifyTimesliceExpired() {

        if (!startedTasks) {
            return;
        }

        elapsedTime_milliseconds += timeslice_milliseconds;

        unblockWakeableTasks();
        scheduleNextTask(); 
        runNextTask();
    }

    void Scheduler::blockTask(BlockReason reason, uint32_t arg) {
        if (reason == BlockReason::Sleep) {

            auto current = currentTask;
            current->state = TaskState::Sleeping;
            currentTask->wakeTime = elapsedTime_milliseconds + arg;

            priorityGroups[static_cast<int>(current->priority)].remove(current);

            if (blockedQueue.isEmpty()) {
                blockedQueue.insertBefore(current, nullptr);
            }
            else {
                
                auto blocked = blockedQueue.getHead();

                while (blocked != nullptr) {
                    if (blocked->wakeTime > current->wakeTime) {

                        blockedQueue.insertBefore(current, blocked);
                        break;
                    }
                    else if (blocked->nextTask == nullptr) {
                       
                        blockedQueue.insertAfter(current, blocked);
                        break;
                    }

                    blocked = blocked->nextTask;
                }
                
            }

            scheduleNextTask();

        }
        else if (reason == BlockReason::WaitingForMessage) {
            auto current = currentTask;
            current->state = TaskState::Blocked;
            priorityGroups[static_cast<int>(current->priority)].remove(current);
            blockedQueue.append(current);

            if (current->priority == Priority::UI 
                && !priorityGroups[static_cast<int>(Priority::IO)].isEmpty()) {
                
                nextTask = priorityGroups[static_cast<int>(Priority::IO)].getHead();
                priorityGroups[static_cast<int>(nextTask->priority)].remove(nextTask);
                scheduleTask(nextTask);
            }
            else {
                scheduleNextTask();
            }
        }

        runNextTask();
    }

    void Scheduler::unblockTask(uint32_t taskId) {
        auto blocked = blockedQueue.getHead();

        while(blocked != nullptr) {
            if (blocked->id == taskId) {
                unblockTask(blocked);
                break;
            }
        }
    }

    void Scheduler::unblockTask(Task* task) {
        task->state = TaskState::Running;

        SpinLock lock {blockedQueue.getLock()};

        blockedQueue.remove(task);
        scheduleTask(task);
    }

    void Scheduler::unblockWakeableTasks() {
        auto blocked = blockedQueue.getHead();

        while(blocked != nullptr) {
            auto next = blocked->nextTask;

            if (blocked->state == TaskState::Sleeping 
                && blocked->wakeTime <= elapsedTime_milliseconds) {
                unblockTask(blocked);
            }
            else if (blocked->state == TaskState::Blocked
                && blocked->mailbox->hasUnreadMessages()) {
                unblockTask(blocked);
            }

            blocked = next;
        }
    }

    void Scheduler::scheduleTask(Task* task) {

        auto& queue = priorityGroups[static_cast<int>(task->priority)];
        SpinLock queueLock {queue.getLock()};

        if (queue.isEmpty()) {
            queue.insertBefore(task, nullptr);
        }
        else {
            queue.append(task);
        }

        task->state = TaskState::Running;
    }

    void Scheduler::setupTimeslice() {
        auto id = CPU::getCurrentCPUId();

        APIC::setAPICTimer(APIC::LVT_TimerMode::Periodic, timeslice_milliseconds);
    }

    void Scheduler::changePriority(Task* task, Priority priority) {

        auto oldPriority = task->priority;
        priorityGroups[static_cast<int>(oldPriority)].remove(task);
        task->priority = priority;
        scheduleTask(task);
    }

    void Scheduler::reschedule() {
        scheduleNextTask();
        runNextTask();
    }

    void Scheduler::sendMessage(IPC::RecipientType recipient, IPC::Message* message) {
        if (currentTask != nullptr) {
            message->senderTaskId = currentTask->id;
        }
        //TODO: implement return value for recipient not found?
        uint32_t taskId {0};

        if (recipient == IPC::RecipientType::ServiceRegistryMailbox) {
            
            ServiceRegistryInstance->receiveMessage(message);
            return;
        }
        else {

            if (recipient == IPC::RecipientType::ServiceName) {
                taskId = ServiceRegistryInstance->getServiceTaskId(message->serviceType);

                if (taskId == 0) {
                    kprintf("[Scheduler] Unknown Service Name\n");
                    return;
                }
            }
            else if (recipient == IPC::RecipientType::Scheduler) {
                taskId = schedulerTask->id;
            }
            else {
                taskId = message->recipientId;
            }

            auto task = TaskStore::getInstance().getTask(taskId);
            task->mailbox->send(message);

            if (task->state == TaskState::Blocked) {
                auto cpuId = CPU::getCurrentCPUId();

                if (cpuId != task->cpuId) {
                    int pause = 0;
                    auto apicId = CPU::ActiveCPUs[task->cpuId].apicId;
                    APIC::sendInterprocessorInterrupt(apicId, APIC::InterprocessorInterrupt::Reschedule);
                }
                else {
                    unblockTask(task);
                }
            }

            if ((static_cast<int>(task->priority) < static_cast<int>(currentTask->priority))
                || currentTask->priority == Priority::IRQ) {
                if (task->state == TaskState::Running) {
                    if (task->cpuId != CPU::getCurrentCPUId()) {
                        auto apicId = CPU::ActiveCPUs[task->cpuId].apicId;
                        APIC::sendInterprocessorInterrupt(apicId, APIC::InterprocessorInterrupt::Reschedule);
                    }
                    else {
                        nextTask = task;
                        runNextTask();
                    }
                }
            }

            return;

        }

        kprintf("[Scheduler] Unsent message from: %d to: %d type: %d\n", message->senderTaskId, taskId, recipient);
        asm("hlt");
    }

    void Scheduler::receiveMessage(IPC::Message* buffer) {
        if (currentTask != nullptr) {
            auto task = currentTask;

            while (!task->mailbox->receive(buffer)) {
                blockTask(BlockReason::WaitingForMessage, 0);
            }
        }
    }

    void Scheduler::receiveMessage(IPC::Message* buffer, IPC::MessageNamespace filter, uint32_t messageId) {
        if (currentTask != nullptr) {
            auto task = currentTask;

            while (true) {

                //wake me up

                if (task->mailbox->filteredReceive(buffer, filter, messageId)) {
                    //wake me up inside
                    return;
                }

                //can't wake up
                blockTask(BlockReason::WaitingForMessage, 0);
            }
        }
    }

    bool Scheduler::peekReceiveMessage(IPC::Message* buffer) {
        if (currentTask != nullptr) {

            return currentTask->mailbox->receive(buffer);
        }

        return false;
    }

    void Scheduler::exitTask() {
        /*
        for usermode tasks, the things we need to cleanup:

        -kernel stack
        -free the PID
        -mailbox
        -free the space in the taskBuffer
        -vmm
        -libc heap
        -user stack

        user stack is at kernelStack - 8
        */
        /*if (false) {
        auto kernelStackAddress = reinterpret_cast<uint32_t>(currentTask->kernelStack);//(currentTask->context.esp & ~0xFFF) + Memory::PageSize;
        auto pageDirectory = currentTask->virtualMemoryManager->getPageDirectory();
        currentTask->virtualMemoryManager->cleanup();
        kernelVMM->activate();
        LibC_Implementation::KernelHeap = kernelHeap;
        kernelVMM->cleanupClonePageTables(pageDirectory, kernelStackAddress - Memory::PageSize);
        delete currentTask->virtualMemoryManager;
        kernelHeap->free(currentTask->mailbox);
        }*/

        //idGenerator.freeId(currentTask->id);
        CurrentTaskLauncher->freeUserTask(currentTask);

        currentTask->state = TaskState::Blocked;
        priorityGroups[static_cast<int>(currentTask->priority)].remove(currentTask);
        scheduleNextTask();
        //readyQueue.remove(currentTask);
        //deleteQueue.append(currentTask);
        //unblockTask(cleanupTask);
        
        runNextTask();
    }

    void Scheduler::exitKernelTask() {
    
        /*
        for kernel tasks, the things we need to cleanup:

        -kernel stack
        -free the PID
        -mailbox
        -free the space in the taskBuffer
        */
        /*kernelVMM->activate();
        LibC_Implementation::KernelHeap = kernelHeap;*/

        //idGenerator.freeId(currentTask->id);
        CurrentTaskLauncher->freeKernelTask(currentTask);

        currentTask->state = TaskState::Blocked;
        priorityGroups[static_cast<int>(currentTask->priority)].remove(currentTask);
        scheduleNextTask();
        //readyQueue.remove(currentTask);
        //deleteQueue.append(currentTask);
        //unblockTask(cleanupTask);
        runNextTask();
    }

    void Scheduler::start() {
        startedTasks = true;

        startTask->virtualMemoryManager = Memory::getCurrentVMM();
        schedulerTask->virtualMemoryManager = Memory::getCurrentVMM();

        scheduleNextTask();
        runNextTask();
    }
}