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
        while(true) {
            //printf("IDLE\n");
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
        startTask = createTestTask(reinterpret_cast<uint32_t>(idleLoop));

        elapsedTime_milliseconds = 0;
        timeslice_milliseconds = 10;
    }

    void Scheduler::scheduleNextTask() {
        if (currentTask == nullptr && !readyQueue.isEmpty()) {
            currentTask = readyQueue.getHead();
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
        
        if (readyQueue.isEmpty()) {
            //nothings ready, did a blocked task's wake time elapse?
            unblockWakeableTasks();

            return readyQueue.getHead();
        }
        else {
            auto next = currentTask->nextTask;

            if (next == nullptr && readyQueue.getHead()->state != TaskState::Sleeping) {
                next = readyQueue.getHead(); 
            }

            return next;
        }
    }

    void Scheduler::runNextTask() {
        if (state == State::StartCurrent) {
            //startProcess(currentTask);
            changeProcess(startTask, currentTask);
        }
        else if (state == State::ChangeToStart) {
            auto current = currentTask;
            currentTask = nullptr;
            changeProcess(current, startTask);
            
        }
        else {
            auto current = currentTask;
            currentTask = nextTask;

            if (current != nextTask) {
                changeProcess(current, nextTask);
            }
        }
    }

    void Scheduler::notifyTimesliceExpired() {
        elapsedTime_milliseconds += timeslice_milliseconds;

        unblockWakeableTasks();
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
        auto userStack = createStack();

        uint8_t volatile* stackPointer = static_cast<uint8_t volatile*>(reinterpret_cast<void volatile*>(processStack + 4096));
        stackPointer -= sizeof(TaskStack);
        //stackPointer += 8;

        TaskStack volatile* stack = reinterpret_cast<TaskStack volatile*>(stackPointer);
        stack->eflags = 
            static_cast<uint32_t>(EFlags::InterruptEnable) | 
            static_cast<uint32_t>(EFlags::Reserved);
        stack->eip = functionAddress;

        Task* task = taskBuffer;
        task->context.esp = reinterpret_cast<uint32_t>(stackPointer);
        task->context.kernelESP = userStack + 4096;

        taskBuffer++;

        return task;
    }

    Task* Scheduler::launchUserProcess(uintptr_t functionAddress) {
        auto kernelStack = createStack();
        auto userStack = createStack();

        uint8_t volatile* stackPointer = static_cast<uint8_t volatile*>(reinterpret_cast<void volatile*>(kernelStack + 4096));
        stackPointer -= sizeof(TaskStack);
        stackPointer -= 8;

        TaskStack volatile* stack = reinterpret_cast<TaskStack volatile*>(stackPointer);
        stack->eflags = 
            static_cast<uint32_t>(EFlags::InterruptEnable) | 
            static_cast<uint32_t>(EFlags::Reserved);
        stack->eip = reinterpret_cast<uint32_t>(launchProcess);
        //stack->eax = functionAddress;

        uint8_t volatile* userStackPointer = static_cast<uint8_t volatile*>(reinterpret_cast<void volatile*>(userStack + 4096));
        userStackPointer -= sizeof(TaskStack);
        TaskStack volatile* uStack = reinterpret_cast<TaskStack volatile*>(userStackPointer);

        uStack->eflags = stack->eflags;
        uStack->eip = functionAddress;

        //stack->ecx = reinterpret_cast<uint32_t>(userStackPointer);
        uint32_t volatile* stackExtras = reinterpret_cast<uint32_t volatile*>(stackPointer + sizeof(TaskStack));
        *stackExtras++ = reinterpret_cast<uint32_t>(userStackPointer);
        *stackExtras++ = functionAddress;

        Task* task = taskBuffer;
        task->context.esp = reinterpret_cast<uint32_t>(stackPointer);
        task->context.kernelESP = kernelStack + 4096;

        taskBuffer++;

        return task;
    }

    void Scheduler::blockThread(BlockReason reason, uint32_t arg) {
        if (reason == BlockReason::Sleep) {

            auto current = currentTask;
            current->state = TaskState::Sleeping;

            currentTask->state = TaskState::Sleeping;
            currentTask->wakeTime = elapsedTime_milliseconds + arg;

            //printf("%x waketime %d duration %d\n", current, (uint32_t)current->wakeTime, arg);

            scheduleNextTask();

            readyQueue.remove(current);

            //insert into blocked queue
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

            runNextTask();
        }
    }

    /*void Scheduler::unblockTask(uint32_t taskId) {
        auto blocked = blockedQueue.getHead();

        while(blocked != nullptr) {
            if (blocked->id == taskId) {
                unblockTask(blocked);
                break;
            }
        }
    }*/

    void Scheduler::unblockTask(Task* task) {
        task->state = TaskState::Running;

        blockedQueue.remove(task);
        scheduleTask(task);
    }

    void Scheduler::unblockWakeableTasks() {
        auto blocked = blockedQueue.getHead();

        while(blocked != nullptr) {
            auto next = blocked->nextTask;

            if (blocked->wakeTime <= elapsedTime_milliseconds) {
                //printf("woke %x waketime %d elapsedtime %d\n", blocked, (uint32_t)blocked->wakeTime, elapsedTime_milliseconds);
                unblockTask(blocked);
            }

            blocked = next;
        }
    }

    void Scheduler::scheduleTask(Task* task) {
        if (readyQueue.isEmpty()) {
            readyQueue.insertBefore(task, nullptr);
        }
        else {
            readyQueue.append(task);
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