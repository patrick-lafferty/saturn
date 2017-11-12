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
        //printf("[Scheduler] Idle\n");
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
            //printf("[Scheduler] Current null\n");
            currentTask = readyQueue;
            //startProcess(currentTask);
            state = State::StartCurrent;
        }
        else {

            auto next = findNextTask();

            if (next == nullptr && currentTask == nullptr) {
                state = State::StartCurrent;
                currentTask = startTask;
            }
            else if (next == nullptr) {
                //no task available
                //enterIdle();
                //printf("[Scheduler] Next = nullptr\n");
                //auto current = currentTask;
                //currentTask = nullptr;
                //changeProcess(current, startTask);
                state = State::ChangeToStart;
            }
            else {
                //printf("[Scheduler] Found next\n");
                //auto current = currentTask;
                //currentTask = next;
                nextTask = next;
                //changeProcess(current, next);
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

            if (next == nullptr) {
                next = readyQueue; //startTask;
            }

            return next;
        }
    }

    void Scheduler::runNextTask() {
        if (state == State::StartCurrent) {
            //printf("[Scheduler] s(c)\n");
            startProcess(currentTask);
        }
        else if (state == State::ChangeToStart) {
            //printf("[Scheduler] c(%x, s)\n", currentTask);
            auto current = currentTask;
            currentTask = nullptr;
            changeProcess(current, startTask);
            
        }
        else {
            //printf("[Scheduler] c(c, n)\n");
            auto current = currentTask;
            currentTask = nextTask;
            //printf("Before change\n");
            changeProcess(current, nextTask);
            //printf("After change\n");
        }
    }

    void Scheduler::notifyTimesliceExpired() {
        elapsedTime_milliseconds += timeslice_milliseconds;

        auto blocked = blockedQueue;

        while(blocked != nullptr) {
            auto next = blocked->nextTask;

            if (blocked->wakeTime <= elapsedTime_milliseconds) {
                blockedQueue = next;

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

        /*auto current = currentTask;
        auto next = currentTask->nextTask;

        if (next == nullptr) {
            next = startTask;
            currentTask = startTask;
        }
        else if (next->state != TaskState::Running) {
            while (next != nullptr) {
                next = next->nextTask;

                if (next->state == TaskState::Running) {
                    break;
                }
            }

            if (next == nullptr) {
                next = startTask;
                currentTask = startTask;
            }
            else {
                currentTask = next;
            }
        }
        else {
            currentTask = currentTask->nextTask;
        }

        changeProcess(current, next);*/
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
        /*stack->cs = 0x1B;
        stack->eflags2 = stack->eflags;
        stack->ss = 0x23;*/

        Task* task = taskBuffer;
        task->context.esp = reinterpret_cast<uint32_t>(stackPointer);
        task->context.kernelESP = processStack + 4096;
        //stack->esp = task->context.esp;
        //stack->esp = processStack + 4096;//task->context.esp;

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

        stack->ecx = reinterpret_cast<uint32_t>(userStackPointer);//userStack + 4096 - sizeof(TaskStack);

        Task* task = taskBuffer;
        task->context.esp = reinterpret_cast<uint32_t>(stackPointer);
        task->context.kernelESP = kernelStack + 4096;

        taskBuffer++;

        return task;
    }

    void Scheduler::blockThread(BlockReason reason, uint32_t arg) {
        if (reason == BlockReason::Sleep) {

            //printf("[Scheduler] Sleeping thread\n");
            auto current = currentTask;

            currentTask->state = TaskState::Sleeping;
            currentTask->wakeTime = elapsedTime_milliseconds + arg;

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
                
                //scheduleNextTask();
            }
            //notifyTimesliceExpired();
            runNextTask();
        }
    }

    void Scheduler::scheduleTask(Task* task) {
        if (readyQueue == nullptr) {
            readyQueue = task;
            //currentTask = task;
        }
        else {
            auto current = readyQueue;//currentTask;

            while (current->nextTask != nullptr) {
                current = current->nextTask;
            }

            current->nextTask = task;
            task->prevTask = current;
        }
    }

    void Scheduler::setupTimeslice() {
        APIC::setAPICTimer(APIC::LVT_TimerMode::Periodic, timeslice_milliseconds);
    }

    void Scheduler::enterIdle() {
        startProcess(startTask);
    }
}