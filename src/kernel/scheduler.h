#pragma once

#include <stdint.h>
#include <task_context.h>

extern "C" uint32_t HACK_TSS_ADDRESS;

namespace Memory {
    class VirtualMemoryManager;
}

namespace LibC_Implementation {
    class Heap;
}

namespace IPC {
    class Mailbox;
    struct Message;
}

namespace Kernel {

    template<typename T> class LinkedList {
        public:
            
            void insertBefore(T* item, T* before) {
                if (head == nullptr) {
                    head = item;
                }
                else {
                    item->nextTask = before;

                    if (before->previousTask != nullptr) {
                        item->previousTask = before->previousTask;
                        before->previousTask->nextTask = item;
                    }
                    else {
                        head = item;
                        item->previousTask = nullptr;
                    }

                    before->previousTask = item;
                }
            }

            void insertAfter(T* item, T* after) {
                after->nextTask = item;
                item->previousTask = after;
            }

            void append(T* item) {
                if (head == nullptr) {
                    head = item;
                    item->previousTask = nullptr;
                }
                else {
                    auto current = head;

                    while (current->nextTask != nullptr) {
                        current = current->nextTask;
                    }

                    current->nextTask = item;
                    item->previousTask = current;
                }

                item->nextTask = nullptr;
            }

            void remove(T* item) {
                auto previous = item->previousTask;
                auto next = item->nextTask;

                if (previous != nullptr) {
                    previous->nextTask = next;
                }
                else {
                    head = next;
                }

                if (next != nullptr) {
                    next->previousTask = previous;
                }

                item->previousTask = nullptr;
                item->nextTask = nullptr;
            }

            T* getHead() {
                return head;
            }

            bool isEmpty() {
                return head == nullptr;
            }

        private:

        T* head {nullptr};
    };

    enum class TaskState {
        Running,
        Sleeping,
        Blocked
    };

    enum class BlockReason {
        Sleep,
        WaitingOnResource,
        WaitingForMessage
    };

    struct Task {
        TaskContext context;
        Task* nextTask {nullptr};
        Task* previousTask {nullptr};
        uint32_t id {0};
        TaskState state;
        uint64_t wakeTime {0};
        Memory::VirtualMemoryManager* virtualMemoryManager;
        LibC_Implementation::Heap* heap;
        IPC::Mailbox* mailbox;
    };

    enum class EFlags {
        Reserved = 1 << 1,
        InterruptEnable = 1 << 9
    };

    struct TaskStack {
        uint32_t eflags;
        uint32_t edi {0};
        uint32_t esi {0};
        uint32_t ebp {0};
        uint32_t ebx {0};
        uint32_t eip {0};
    };

    //TODO: HACK: really have to find a way to store the schedulers
    extern class Scheduler* currentScheduler;

    enum class State {
        StartCurrent,
        ChangeToStart,
        ChangeToNext
    };

    class Scheduler {
    public:

        Scheduler();

        Task* createKernelTask(uintptr_t functionAddress);
        Task* createUserTask(uintptr_t functionAddress);
        
        void notifyTimesliceExpired();
        void scheduleTask(Task* task);

        void blockTask(BlockReason reason, uint32_t arg);
        void unblockTask(uint32_t taskId);

        void enterIdle();
        void setupTimeslice();

        void sendMessage(uint32_t taskId, IPC::Message* message);
        void receiveMessage(IPC::Message* buffer);

        Task* getTask(uint32_t taskId);

    private:

        void scheduleNextTask();
        Task* findNextTask();
        void runNextTask();
        
        void unblockTask(Task* task);
        void unblockWakeableTasks();

        Task idleTask;
        Task* taskBuffer;
        Task* currentTask {nullptr};
        Task* nextTask {nullptr};
        LinkedList<Task> readyQueue;
        LinkedList<Task> blockedQueue;
        Task* startTask;

        uint64_t elapsedTime_milliseconds;
        uint32_t timeslice_milliseconds;

        State state {State::StartCurrent};
    };
}

