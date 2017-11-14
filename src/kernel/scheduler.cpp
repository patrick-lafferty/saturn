#include "scheduler.h"
#include <stdio.h>
#include <memory/physical_memory_manager.h>
#include <memory/virtual_memory_manager.h>
#include <cpu/apic.h>

extern "C" void startProcess(Kernel::Task* task);
extern "C" void changeProcess(Kernel::Task* current, Kernel::Task* next);
extern "C" void launchProcess();

extern "C" void printLaunchProcess() {
    printf("launchProcess\n");
}

namespace Kernel {

    Scheduler* currentScheduler;

    void idleLoop() {
        while(true) {
            asm volatile("hlt");
        }
    }

    Scheduler::Scheduler() {
        currentScheduler = this;

        auto taskBufferAddress = Memory::currentVMM->allocatePages(1, static_cast<int>(Memory::PageTableFlags::Present)
            | static_cast<int>(Memory::PageTableFlags::AllowWrite));
        auto physicalPage = Memory::currentPMM->allocatePage(1);
        Memory::currentVMM->map(taskBufferAddress, physicalPage);
        Memory::currentPMM->finishAllocation(taskBufferAddress, 1);
        
        taskBuffer = static_cast<Task*>(reinterpret_cast<void*>(taskBufferAddress));
        currentTask = createTestTask(reinterpret_cast<uint32_t>(idleLoop));
        startTask = currentTask;

        elapsedTime_milliseconds = 0;
        timeslice_milliseconds = 10;
    }

    void Scheduler::scheduleNextTask() {
        if (currentTask == nullptr && readyQueue != nullptr) {
            currentTask = readyQueue;
            state = State::StartCurrent;
        }
        else {

            auto next = findNextTask();

            if (next == nullptr && currentTask == nullptr) {
                state = State::StartCurrent;
                currentTask = startTask;
            }
            else if (next == nullptr) {
                state = State::ChangeToStart;
            }
            else {
                nextTask = next;
                state = State::ChangeToNext;
            }
        }
    }

    Task* Scheduler::findNextTask() {
        
        if (readyQueue == nullptr) {
            //nothings ready, did a blocked task's wake time elapse?
            auto blocked = blockedQueue;

            while(blocked != nullptr) {
                auto next = blocked->nextTask;

                if (blocked->wakeTime <= elapsedTime_milliseconds) {
                    blockedQueue = next;
                printf("woke %x waketime %d elapsedtime %d\n", blocked, (uint32_t)blocked->wakeTime, elapsedTime_milliseconds);

                    if (blocked->prevTask != nullptr) {
                        blocked->prevTask->nextTask = next;
                    }

                    if (next != nullptr) {
                        next->prevTask = blocked->prevTask;
                    }

                    blocked->nextTask = nullptr;
                    blocked->prevTask = nullptr;
                    
                    scheduleTask(blocked);
                }

                blocked = next;
            }

            return readyQueue;
        }
        else {
            auto next = currentTask->nextTask;

            if (next == nullptr && readyQueue->state != TaskState::Sleeping) {
                next = readyQueue; 
            }

            return next;
        }
    }

    void Scheduler::runNextTask() {
        if (state == State::StartCurrent) {
            startProcess(currentTask);
        }
        else if (state == State::ChangeToStart) {
            auto current = currentTask;
            currentTask = nullptr;
            changeProcess(current, startTask);
            
        }
        else {
            auto current = currentTask;
            currentTask = nextTask;
            changeProcess(current, nextTask);
        }
    }

    void Scheduler::notifyTimesliceExpired() {
        elapsedTime_milliseconds += timeslice_milliseconds;

        auto blocked = blockedQueue;

        while(blocked != nullptr) {
            auto next = blocked->nextTask;

            if (blocked->wakeTime <= elapsedTime_milliseconds) {
                blockedQueue = next;
                printf("woke %x waketime %d elapsedtime %d\n", blocked, (uint32_t)blocked->wakeTime, elapsedTime_milliseconds);

                if (blocked->prevTask != nullptr) {
                    blocked->prevTask->nextTask = next;
                }

                if (next != nullptr) {
                    next->prevTask = blocked->prevTask;
                }

                blocked->nextTask = nullptr;
                blocked->prevTask = nullptr;
                
                scheduleTask(blocked);
            }

            blocked = next;
        }

        scheduleNextTask(); 
        runNextTask();
    }

    uint32_t createStack() {
        auto processStack = Memory::currentVMM->allocatePages(1, 
            static_cast<int>(Memory::PageTableFlags::Present)
            | static_cast<int>(Memory::PageTableFlags::AllowWrite)
            | static_cast<int>(Memory::PageTableFlags::AllowUserModeAccess));
        auto physicalPage = Memory::currentPMM->allocatePage(1);
        Memory::currentVMM->map(processStack, physicalPage);
        Memory::currentPMM->finishAllocation(processStack, 1);

        return processStack;
    }

    Task* Scheduler::createTestTask(uintptr_t functionAddress) {
        auto processStack = createStack();

        uint8_t volatile* stackPointer = static_cast<uint8_t volatile*>(reinterpret_cast<void volatile*>(processStack + 4096));
        stackPointer -= sizeof(TaskStack);

        TaskStack volatile* stack = reinterpret_cast<TaskStack volatile*>(stackPointer);
        stack->eflags = 
            static_cast<uint32_t>(EFlags::InterruptEnable) | 
            static_cast<uint32_t>(EFlags::Reserved);
        stack->eip = functionAddress;

        Task* task = taskBuffer;
        task->context.esp = reinterpret_cast<uint32_t>(stackPointer);
        task->context.kernelESP = processStack + 4096;

        taskBuffer++;

        return task;
    }

    Task* Scheduler::launchUserProcess(uintptr_t functionAddress) {
        auto kernelStack = createStack();
        auto userStack = createStack();

        uint8_t volatile* stackPointer = static_cast<uint8_t volatile*>(reinterpret_cast<void volatile*>(kernelStack + 4096));
        stackPointer -= sizeof(TaskStack);

        TaskStack volatile* stack = reinterpret_cast<TaskStack volatile*>(stackPointer);
        stack->eflags = 
            static_cast<uint32_t>(EFlags::InterruptEnable) | 
            static_cast<uint32_t>(EFlags::Reserved);
        stack->eip = reinterpret_cast<uint32_t>(launchProcess);
        stack->eax = functionAddress;

        uint8_t volatile* userStackPointer = static_cast<uint8_t volatile*>(reinterpret_cast<void volatile*>(userStack + 4096));
        userStackPointer -= sizeof(TaskStack);
        TaskStack volatile* uStack = reinterpret_cast<TaskStack volatile*>(userStackPointer);

        uStack->eflags = stack->eflags;
        uStack->eip = functionAddress;

        stack->ecx = reinterpret_cast<uint32_t>(userStackPointer);

        Task* task = taskBuffer;
        task->context.esp = reinterpret_cast<uint32_t>(stackPointer);
        task->context.kernelESP = kernelStack + 4096;

        taskBuffer++;

        return task;
    }

    void Scheduler::blockThread(BlockReason reason, uint32_t arg) {
        if (reason == BlockReason::Sleep) {

            auto current = currentTask;

            currentTask->state = TaskState::Sleeping;
            currentTask->wakeTime = elapsedTime_milliseconds + arg;

            printf("%x waketime %d duration %d\n", current, (uint32_t)current->wakeTime, arg);

            scheduleNextTask();

            {
                auto prev = current->prevTask;
                auto next = current->nextTask;

                //remove from ready queue
                if (prev != nullptr) {
                    prev->nextTask = current->nextTask;
                }
                else {
                    readyQueue = next;
                }

                if (next != nullptr) {
                    next->prevTask = prev;
                }
            }

            current->nextTask = nullptr;
            current->prevTask = nullptr;

            //insert into blocked queue
            if (blockedQueue == nullptr) {
                blockedQueue = current;
            }
            else {
                
                auto blocked = blockedQueue;

                while (blocked != nullptr) {
                    if (blocked->wakeTime > current->wakeTime) {
                        current->nextTask = blocked;

                        if (blocked->prevTask != nullptr) {
                            current->prevTask = blocked->prevTask;
                            blocked->prevTask->nextTask = current;
                        }
                        else {
                            blockedQueue = current;
                            current->prevTask = nullptr;
                        }

                        blocked->prevTask = current;

                        break;
                    }
                    else {
                        if (blocked->nextTask == nullptr) {
                            blocked->nextTask = current;
                            current->prevTask = blocked;
                            break;
                        }
                    }

                    blocked = blocked->nextTask;
                }
                
            }

            runNextTask();
        }
    }

    void Scheduler::scheduleTask(Task* task) {
        if (readyQueue == nullptr) {
            readyQueue = task;
        }
        else {
            auto current = readyQueue;

            while (current->nextTask != nullptr) {
                current = current->nextTask;
            }

            current->nextTask = task;
            task->prevTask = current;
        }

        task->state = TaskState::Running;
    }

    void Scheduler::setupTimeslice() {
        APIC::setAPICTimer(APIC::LVT_TimerMode::Periodic, timeslice_milliseconds);
    }

    void Scheduler::enterIdle() {
        startProcess(startTask);
    }
}