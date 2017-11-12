#include "scheduler.h"
#include <stdio.h>
#include <memory/physical_memory_manager.h>
#include <memory/virtual_memory_manager.h>
#include <cpu/apic.h>

extern "C" void startProcess(Kernel::Task* task);
extern "C" void changeProcess(Kernel::Task* current, Kernel::Task* next);
extern "C" void launchProcess();
namespace Kernel {

    Scheduler* currentScheduler;

    void idleLoop() {
        //printf("[Scheduler] Idle\n");
        asm volatile("hlt");
        idleLoop();
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
        timeslice_milliseconds = 100;
    }

    void Scheduler::scheduleNextTask() {
        if (currentTask == nullptr && readyQueue != nullptr) {
            printf("[Scheduler] Current null\n");
            currentTask = readyQueue;
            startProcess(currentTask);
        }
        else {

            auto next = findNextTask();

            if (next == nullptr) {
                //no task available
                //enterIdle();
                printf("[Scheduler] Next = nullptr\n");
                auto current = currentTask;
                currentTask = nullptr;
                changeProcess(current, startTask);
            }
            else {
                printf("[Scheduler] Found next\n");
                auto current = currentTask;
                currentTask = next;
                changeProcess(current, next);
            }
        }
    }

    Task* Scheduler::findNextTask() {
        
        if (readyQueue == nullptr) {
            //nothings ready, did a blocked task's wake time elapse?
            auto blocked = blockedQueue;

            while(blocked != nullptr) {
                if (blocked->wakeTime >= elapsedTime_milliseconds) {
                    readyQueue = blocked;
                    blockedQueue = blocked->nextTask;

                    if (blocked->prevTask != nullptr) {
                        blocked->prevTask = blocked->nextTask;
                    }
                    
                    break;
                }

                blocked = blocked->nextTask;
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

    void Scheduler::notifyTimesliceExpired() {
        elapsedTime_milliseconds += timeslice_milliseconds;

        scheduleNextTask(); 

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

    Task* Scheduler::createTestTask(uintptr_t functionAddress) {
        auto processStack = Memory::currentVMM->allocatePages(1, 
            static_cast<int>(Memory::PageTableFlags::Present)
            | static_cast<int>(Memory::PageTableFlags::AllowWrite)
            | static_cast<int>(Memory::PageTableFlags::AllowUserModeAccess));
        auto physicalPage = Memory::currentPMM->allocatePage(1);
        Memory::currentVMM->map(processStack, physicalPage);
        Memory::currentPMM->finishAllocation(processStack, 1);

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
        //stack->esp = task->context.esp;
        //stack->esp = processStack + 4096;//task->context.esp;

        taskBuffer++;

        return task;
    }

    Task* Scheduler::launchUserProcess(uintptr_t functionAddress) {
        auto processStack = Memory::currentVMM->allocatePages(1, 
            static_cast<int>(Memory::PageTableFlags::Present)
            | static_cast<int>(Memory::PageTableFlags::AllowWrite)
            | static_cast<int>(Memory::PageTableFlags::AllowUserModeAccess));
        auto physicalPage = Memory::currentPMM->allocatePage(1);
        Memory::currentVMM->map(processStack, physicalPage);
        Memory::currentPMM->finishAllocation(processStack, 1);

        uint8_t volatile* stackPointer = static_cast<uint8_t volatile*>(reinterpret_cast<void volatile*>(processStack + 4096));
        stackPointer -= sizeof(TaskStack);

        TaskStack volatile* stack = reinterpret_cast<TaskStack volatile*>(stackPointer);
        stack->eflags = 
            static_cast<uint32_t>(EFlags::InterruptEnable) | 
            static_cast<uint32_t>(EFlags::Reserved);
        stack->eip = reinterpret_cast<uint32_t>(launchProcess);
        stack->eax = functionAddress;

        Task* task = taskBuffer;
        task->context.esp = reinterpret_cast<uint32_t>(stackPointer);

        taskBuffer++;

        return task;
    }

    void Scheduler::blockThread(BlockReason reason, uint32_t arg) {
        if (reason == BlockReason::Sleep) {
            currentTask->state = TaskState::Sleeping;
            currentTask->wakeTime = elapsedTime_milliseconds + arg;

            //remove from ready queue
            if (currentTask->prevTask != nullptr) {
                auto previous = currentTask->prevTask;
                previous->nextTask = currentTask->nextTask;
                currentTask->nextTask = previous;
                //currentTask->prevTask = currentTask->nextTask;
                //currentTask->nextTask->prevTask = previous;
            }
            else {
                readyQueue = currentTask->nextTask;
                //currentTask->prevTask = nullptr;
            }

            //insert into blocked queue
            if (blockedQueue == nullptr) {
                blockedQueue = currentTask;
            }
            else {
                auto blocked = blockedQueue;

                while (blocked != nullptr) {
                    if (blocked->wakeTime >= currentTask->wakeTime) {
                        currentTask->nextTask = blocked;

                        if (blocked->prevTask != nullptr) {
                            currentTask->prevTask = blocked->prevTask;
                            blocked->prevTask->nextTask = currentTask;
                        }
                        else {
                            blockedQueue = currentTask;
                            currentTask->prevTask = nullptr;
                        }

                        blocked->prevTask = currentTask;

                        break;
                    }

                    if (blocked->nextTask == nullptr) {
                        blocked->nextTask = currentTask;
                        currentTask->prevTask = blocked;
                        break;
                    }
                }
                
                scheduleNextTask();
            }
            //notifyTimesliceExpired();
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