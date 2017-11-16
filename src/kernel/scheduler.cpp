#include "scheduler.h"
#include <stdio.h>
#include <memory/physical_memory_manager.h>
#include <memory/virtual_memory_manager.h>
#include <cpu/apic.h>

extern "C" void startProcess(Kernel::Task* task);
extern "C" void changeProcess(Kernel::Task* current, Kernel::Task* next);
extern "C" void launchProcess();

uint32_t HACK_TSS_ADDRESS;

namespace Kernel {

    Scheduler* currentScheduler;

    void idleLoop() {
        while(true) {
            printf("Idle\n");
            asm volatile("hlt");
        }
    }

    uint32_t createStack(bool allowUserMode) {
        auto flags = static_cast<int>(Memory::PageTableFlags::Present)
            | static_cast<int>(Memory::PageTableFlags::AllowWrite);

        if (allowUserMode) {
            flags |= static_cast<int>(Memory::PageTableFlags::AllowUserModeAccess);
        }

        auto processStack = Memory::currentVMM->allocatePages(1, flags);
        auto physicalPage = Memory::currentPMM->allocatePage(1);
        Memory::currentVMM->map(processStack, physicalPage);
        Memory::currentPMM->finishAllocation(processStack, 1);

        return processStack;
    }

    Scheduler::Scheduler() {
        currentScheduler = this;

        auto taskBufferAddress = createStack(false);
        
        taskBuffer = static_cast<Task*>(reinterpret_cast<void*>(taskBufferAddress));
        startTask = createKernelTask(reinterpret_cast<uint32_t>(idleLoop));

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
    
    Task* Scheduler::createKernelTask(uintptr_t functionAddress) {
        auto processStack = createStack(false);

        uint8_t volatile* stackPointer = static_cast<uint8_t volatile*>(reinterpret_cast<void volatile*>(processStack + 4096));
        stackPointer -= sizeof(TaskStack);

        TaskStack volatile* stack = reinterpret_cast<TaskStack volatile*>(stackPointer);
        stack->eflags = 
            static_cast<uint32_t>(EFlags::InterruptEnable) | 
            static_cast<uint32_t>(EFlags::Reserved);
        stack->eip = functionAddress;

        Task* task = taskBuffer;
        task->context.esp = reinterpret_cast<uint32_t>(stackPointer);
        task->context.kernelESP = task->context.esp;

        taskBuffer++;

        return task;
    }

    /*
    When creating a user task, we need to push additional arguments to the stack
    that launchProcess uses to run the actual user code. So we need to move
    the kernel stack up a bit to fit those arguments at the bottom of the stack.
    */
    uint8_t volatile* adjustStack(Task* task) {
        auto address = task->context.esp;
        uint8_t volatile* stackPointer = static_cast<uint8_t volatile*>
            (reinterpret_cast<void volatile*>(address));

        TaskStack volatile* stack = reinterpret_cast<TaskStack volatile*>(stackPointer);

        auto eflags = stack->eflags;
        auto eip = stack->eip;
        
        for(auto i = 0u; i < sizeof(TaskStack); i++) {
            *stackPointer = 0;
        }

        stackPointer -= 8;
        stack = reinterpret_cast<TaskStack volatile*>(stackPointer);

        stack->eflags = eflags;
        stack->eip = eip;

        task->context.esp = reinterpret_cast<uint32_t>(stackPointer);

        return stackPointer + sizeof(TaskStack);
    }

    Task* Scheduler::createUserTask(uintptr_t functionAddress) {
        auto task = createKernelTask(reinterpret_cast<uintptr_t>(launchProcess));
        auto userStackAddress = createStack(true);

        /*
        Need to adjust the kernel stack because we want to add
        userESP and userEIP to the top
        */
        auto kernelStackPointer = adjustStack(task);

        uint8_t volatile* userStackPointer = static_cast<uint8_t volatile*>
            (reinterpret_cast<void volatile*>(userStackAddress + 4096));
        userStackPointer -= sizeof(TaskStack);
        TaskStack volatile* userStack = reinterpret_cast<TaskStack volatile*>
            (userStackPointer);

        userStack->eflags = reinterpret_cast<TaskStack volatile*>
            (kernelStackPointer - sizeof(TaskStack))->eflags;
        userStack->eip = functionAddress;
        
        /*
        createUserTask creates a task that starts inside launchProcess,
        which pops off two values off the stack (user ESP and user EIP)
        and then enters the user task. So we need to write those two values
        here
        */
        auto stackExtras = reinterpret_cast<uint32_t volatile*>(kernelStackPointer);
        *stackExtras++ = reinterpret_cast<uint32_t>(userStackPointer);
        *stackExtras++ = functionAddress;

        return task;
    }

    void Scheduler::blockTask(BlockReason reason, uint32_t arg) {
        if (reason == BlockReason::Sleep) {

            auto current = currentTask;
            current->state = TaskState::Sleeping;

            currentTask->state = TaskState::Sleeping;
            currentTask->wakeTime = elapsedTime_milliseconds + arg;

            scheduleNextTask();

            readyQueue.remove(current);

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

        blockedQueue.remove(task);
        scheduleTask(task);
    }

    void Scheduler::unblockWakeableTasks() {
        auto blocked = blockedQueue.getHead();

        while(blocked != nullptr) {
            auto next = blocked->nextTask;

            if (blocked->wakeTime <= elapsedTime_milliseconds) {
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