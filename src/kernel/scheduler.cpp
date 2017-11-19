#include "scheduler.h"
#include <stdio.h>
#include <memory/physical_memory_manager.h>
#include <memory/virtual_memory_manager.h>
#include <cpu/apic.h>
#include <heap.h>
#include "ipc.h"
#include <new.h>
#include <services.h>

extern "C" void startProcess(Kernel::Task* task);
extern "C" void changeProcess(Kernel::Task* current, Kernel::Task* next);
extern "C" void launchProcess();

uint32_t HACK_TSS_ADDRESS;

namespace Kernel {

    Scheduler* currentScheduler;

    void idleLoop() {
        while(true) {
            asm volatile("hlt");
        }
    }

    struct alignas(0x1000) Stack {

    };

    uint32_t createStack(bool) {
        auto address = new Stack;
        return reinterpret_cast<uint32_t>(address);
    }

    Scheduler::Scheduler() {
        currentScheduler = this;

        taskBuffer = new Task[10];
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

            if (next == nullptr && readyQueue.getHead()->state == TaskState::Running) {
                next = readyQueue.getHead(); 
            }

            return next;
        }
    }

    void Scheduler::runNextTask() {
        if (state == State::StartCurrent) {
            LibC_Implementation::KernelHeap = currentTask->heap;
            changeProcess(startTask, currentTask);
        }
        else if (state == State::ChangeToStart) {
            auto current = currentTask;
            currentTask = nullptr;
            LibC_Implementation::KernelHeap = startTask->heap;
            changeProcess(current, startTask);
            
        }
        else {
            auto current = currentTask;
            currentTask = nextTask;

            if (current != nextTask) {
                LibC_Implementation::KernelHeap = nextTask->heap;
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

        static uint32_t nextTaskId {0};
        Task* task = taskBuffer;
        task->id = nextTaskId++;
        task->context.esp = reinterpret_cast<uint32_t>(stackPointer);
        task->context.kernelESP = task->context.esp;
        task->virtualMemoryManager = Memory::currentVMM;
        task->heap = LibC_Implementation::KernelHeap;

        const uint32_t mailboxSize = Memory::PageSize;
        //auto mailboxAddress = Memory::currentVMM->allocatePages(1, static_cast<uint32_t>(Memory::PageTableFlags::AllowWrite));
        auto mailboxMemory = task->heap->allocate(mailboxSize);

        task->mailbox = new (mailboxMemory) 
            IPC::Mailbox(reinterpret_cast<uint32_t>(mailboxMemory) + sizeof(IPC::Mailbox), 
            //IPC::Mailbox(mailboxAddress + sizeof(IPC::Mailbox), 
            mailboxSize - sizeof(IPC::Mailbox));


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

        stackPointer -= 12;
        stack = reinterpret_cast<TaskStack volatile*>(stackPointer);

        stack->eflags = eflags;
        stack->eip = eip;

        task->context.esp = reinterpret_cast<uint32_t>(stackPointer);

        return stackPointer + sizeof(TaskStack);
    }

    Task* Scheduler::createUserTask(uintptr_t functionAddress) {
        auto task = createKernelTask(reinterpret_cast<uintptr_t>(launchProcess));
        auto oldVMM = Memory::currentVMM;
        auto vmm = Memory::currentVMM->cloneForUsermode();
        task->virtualMemoryManager = vmm;
        vmm->HACK_setNextAddress(0xd000'0000 - 0x2000);
        auto backupHeap = LibC_Implementation::KernelHeap;
        LibC_Implementation::KernelHeap = nullptr;
        vmm->activate();
        auto userStackAddress = createStack(true);
        vmm->HACK_setNextAddress(0xa000'0000);
        task->heap = LibC_Implementation::KernelHeap;
        task->heap->HACK_syncPageWithVMM();

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
        *stackExtras++ = reinterpret_cast<uint32_t>(vmm);
        *stackExtras++ = reinterpret_cast<uint32_t>(userStackPointer);
        *stackExtras++ = functionAddress;

        oldVMM->activate();
        LibC_Implementation::KernelHeap = backupHeap;

        return task;
    }

    void Scheduler::blockTask(BlockReason reason, uint32_t arg) {
        if (reason == BlockReason::Sleep) {

            auto current = currentTask;
            current->state = TaskState::Sleeping;
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

        }
        else if (reason == BlockReason::WaitingForMessage) {
            auto current = currentTask;
            current->state = TaskState::Blocked;
            scheduleNextTask();
            readyQueue.remove(current);
            blockedQueue.append(currentTask);
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

    void Scheduler::sendMessage(IPC::RecipientType recipient, IPC::Message* message) {
        message->senderTaskId = currentTask->id;

        if (recipient == IPC::RecipientType::ServiceRegistryMailbox) {
            ServiceRegistryInstance->receiveMessage(message);
        }
        else {
            uint32_t taskId {0};

            if (recipient == IPC::RecipientType::ServiceName) {
                taskId = ServiceRegistryInstance->getServiceTaskId(message->serviceType);
            }
            else {
                taskId = message->recipientId;
            }
            //check blocked tasks first

            if (!blockedQueue.isEmpty()) {
                auto task = blockedQueue.getHead();

                while (task != nullptr) {

                    if (task->id == taskId) {
                        //TODO: check if mailbox is full, if so block current task
                        task->mailbox->send(message);
                        
                        if (task->state == TaskState::Blocked) {
                            unblockTask(task);
                        }

                        return;
                    }

                    task = task->nextTask;
                }
            }
            else {
                auto task = readyQueue.getHead();

                while (task != nullptr) {
                    if (task->id == taskId) {
                        //TODO: check if mailbox is full, if so block current task
                        task->mailbox->send(message);
                        return;
                    }

                    task = task->nextTask;
                }
            }
        }
    }

    void Scheduler::receiveMessage(IPC::Message* buffer) {
        if (currentTask != nullptr) {
            if (!currentTask->mailbox->hasUnreadMessages()) {
                blockTask(BlockReason::WaitingForMessage, 0);
            }

            currentTask->mailbox->receive(buffer);
        }
    }

    Task* Scheduler::getTask(uint32_t taskId) {
        if (!readyQueue.isEmpty()) {
            auto task = readyQueue.getHead();

            while (task != nullptr) {
                if (task->id == taskId) {
                    return task;
                }

                task = task->nextTask;
            }
        }

        if (!blockedQueue.isEmpty()) {
            auto task = blockedQueue.getHead();

            while (task != nullptr) {
                if (task->id == taskId) {
                    return task;
                }

                task = task->nextTask;
            }
        }

        return nullptr;
    }

    void Scheduler::enterIdle() {
        startProcess(startTask);
    }
}