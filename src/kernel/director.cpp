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

#include "director.h"
#include <cpu/cpu.h>
#include "task.h"
#include "scheduler.h"
#include <locks.h>

namespace Kernel {

    //for debugging only
    Director* CurrDirector {nullptr};

    Director& Director::GetInstance() {
        static Director instance;
        if (CurrDirector == nullptr) CurrDirector = &instance;
        return instance;
    }

    void Director::scheduleTask(Task* task) {

        int minScore = 10;
        auto scheduler = CPU::ActiveCPUs[0].scheduler;

        for (int i = 0; i < CPU::CPUCount; i++) {
            int score = 10;
            auto currentScheduler = CPU::ActiveCPUs[i].scheduler;
            
            if (i == task->cpuId) {
                score--;
            }

            score += currentScheduler->getPriorityScore(task);

            if (score < minScore) {
                minScore = score;
                scheduler = currentScheduler;
            }
        }

        scheduler->scheduleTask(task);
    }

    void Director::blockTask(Task* task, BlockReason reason) {
        task->state = TaskState::Blocked;

        SpinLock lock {blockedTasks.getLock()};
        blockedTasks.append(task);
    }

    void Director::sleepTask(Task* task, int time) {
        SpinLock lock {sleepingTasks.getLock()};
        auto blocked = sleepingTasks.getHead();

        while (blocked != nullptr) {
            if (blocked->wakeTime > task->wakeTime) {

                sleepingTasks.insertBefore(task, blocked);
                break;
            }
            else if (blocked->nextTask == nullptr) {
                
                sleepingTasks.insertAfter(task, blocked);
                break;
            }

            blocked = blocked->nextTask;
        }
    }

    void Director::unblockTask(Task* task) {
        task->state = TaskState::ReadyToRun;

        SpinLock lock {blockedTasks.getLock()};
        blockedTasks.remove(task);

        scheduleTask(task);
    }

    void Director::unblockWakeableTasks() {
        SpinLock lock {blockedTasks.getLock()};
        auto blocked = blockedTasks.getHead(); 

        while (blocked != nullptr) {
            auto next = blocked->nextTask;

            if (blocked->mailbox->hasUnreadMessages()) {
                blockedTasks.remove(blocked);
                blocked->state = TaskState::ReadyToRun;
                scheduleTask(blocked);
            }

            blocked = next;
        }
    }

}
