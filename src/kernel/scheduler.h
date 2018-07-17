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
#pragma once

#include <stdint.h>
#include <task_context.h>
#include <vector>
#include "list.h"

namespace Memory {
    class VirtualMemoryManager;
}

namespace IPC {
    class Mailbox;
    struct Message;
    enum class RecipientType;
    enum class MessageNamespace : uint32_t;
}

namespace Kernel {

    void schedulerService();

    enum class EFlags {
        Reserved = 1 << 1,
        InterruptEnable = 1 << 9
    };

    enum class State {
        StartCurrent,
        ChangeToStart,
        ChangeToNext
    };

    struct Task;
    enum class Priority;

    class Scheduler {
    public:

        Scheduler();

        void notifyTimesliceExpired();
        void scheduleTask(Task* task);

        //void blockTask(BlockReason reason, uint32_t arg);
        //void unblockTask(uint32_t taskId);
        void sleepTask(int time);

        void start();
        void setupTimeslice();

        void sendMessage(IPC::RecipientType recipient, IPC::Message* message);
        void receiveMessage(IPC::Message* buffer);
        void receiveMessage(IPC::Message* buffer, IPC::MessageNamespace filter, uint32_t messageId);
        bool peekReceiveMessage(IPC::Message* buffer);

        void exitTask();
        void exitKernelTask();

        void cleanupTasks();

        Task* getCurrentTask() {
            return currentTask;
        }

        void changePriority(Task* task, Priority priority);
        void reschedule();

        int getPriorityScore(Task* pending);

    private:

        void scheduleNextTask();
        Task* findNextTask();
        void runNextTask();
        
        void unblockTask(Task* task);
        void unblockWakeableTasks();

        Task* currentTask {nullptr};
        Task* nextTask {nullptr};
        Task* startTask {nullptr};
        Task* cleanupTask {nullptr};
        //Task* schedulerTask {nullptr};
        LinkedList<Task> readyQueue;
        LinkedList<Task> sleepingTasks;
        //LinkedList<Task> deleteQueue;

        LinkedList<Task> priorityGroups[6];

        uint64_t elapsedTime_milliseconds;
        uint32_t timeslice_milliseconds;

        State state {State::StartCurrent};

        bool startedTasks {false};
    };
}

