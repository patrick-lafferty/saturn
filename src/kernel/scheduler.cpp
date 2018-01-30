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
#include <heap.h>
#include "ipc.h"
#include <new>
#include <services.h>
#include <stdlib.h>
#include <cpu/tss.h>
#include <system_calls.h>

extern "C" void startProcess(Kernel::Task* task);
extern "C" void changeProcess(Kernel::Task* current, Kernel::Task* next);
extern "C" void launchProcess();
extern "C" void usermodeStub();
extern "C" void elfUsermodeStub();

uint32_t HACK_TSS_ADDRESS;

struct ElfHeader {
    uint32_t magicNumber;
    uint8_t bitFormat;
    uint8_t endianness;
    uint8_t versionShort;
    uint8_t osABI;
    uint8_t abiVersion;
    uint8_t pad[7];
    uint16_t type;
    uint16_t machine;
    uint32_t versionLong;
    uint32_t entryPoint;
    uint32_t programHeaderStart;
    uint32_t sectionHeaderStart;
    uint32_t flags;
    uint16_t headerSize;
    uint16_t programHeaderEntrySize;
    uint16_t programHeaderEntryCount;
    uint16_t sectionHeaderEntrySize;
    uint16_t sectionHeaderEntryCount;
    uint16_t sectionHeaderNameIndex;
};

struct ProgramHeader {
    uint32_t type;
    uint32_t offset;
    uint32_t virtualAddress;
    uint32_t physicalAddress;
    uint32_t imageSize;
    uint32_t memorySize;
    uint32_t flags;
    uint32_t alignment;
};

extern "C" uint32_t loadElf(char* pah) {
    char* path = "/applications/test/test.bin";
    auto file = fopen(path, "");
#if 0
    char* data = new char[0x2000];

    fread(data, 0x2000, 1, file);

    return 0;
#endif

#if 1
    ElfHeader header;
    fread(&header, sizeof(header), 1, file);
    fseek(file, header.programHeaderStart, SEEK_SET);
    ProgramHeader programs[3];
    ProgramHeader program;

    for (auto i = 0u; i < header.programHeaderEntryCount; i++) {
        fread(&programs[i], sizeof(program), 1, file);
    }

    program = programs[0]; 

    /*auto savedAddress = Memory::currentVMM->HACK_getNextAddress();

    Memory::currentVMM->HACK_setNextAddress(program.virtualAddress);
    auto addr = Memory::currentVMM->allocatePages(program.imageSize / Memory::PageSize,
        static_cast<uint32_t>(Memory::PageTableFlags::AllowUserModeAccess));

    uint8_t* prog = reinterpret_cast<uint8_t*>(addr);*/
    auto prog = reinterpret_cast<uint8_t*>(map(program.virtualAddress, 
        1 + program.imageSize / Memory::PageSize,
        static_cast<uint32_t>(Memory::PageTableFlags::AllowUserModeAccess)
            //| static_cast<uint32_t>(Memory::PageTableFlags::Present)
            | static_cast<uint32_t>(Memory::PageTableFlags::AllowWrite)));

        /*map(program.virtualAddress + 0x1000, 
        1,
        static_cast<uint32_t>(Memory::PageTableFlags::AllowUserModeAccess)
            //| static_cast<uint32_t>(Memory::PageTableFlags::Present)
            | static_cast<uint32_t>(Memory::PageTableFlags::AllowWrite));*/
    //*prog = 0;
    //*(prog + 0x1000) = 0;
    //fseek(file, header.programHeaderStart + header.programHeaderEntrySize * header.programHeaderEntryCount,
    //    SEEK_SET);
    fread(prog + 148, program.imageSize, 1, file);

    //Memory::currentVMM->HACK_setNextAddress(savedAddress);

    return header.entryPoint;// - (sizeof(header) + sizeof(program) * 3); //0xa0;
    #endif
}

extern "C" void idleLoop();

namespace Kernel {

    Scheduler* currentScheduler;

    void cleanupTasksService() {
        currentScheduler->cleanupTasks();
    }

    void callExitKernelTask() {
        currentScheduler->exitKernelTask();
    }

    struct alignas(0x1000) Stack {
        char data[0x100000];
    };

    uint32_t IdGenerator::generateId() {
        int blockId = 0;

        for (auto& block : idBitmap) {
            if (block != 0) {
                uint32_t bit {0};

                asm ("tzcnt %0, %1"
                    : "=r" (bit)
                    : "r" (block));

                auto id = blockId * idsPerBlock + bit;
                block &= ~(1 << bit);

                return id;
            }

            blockId++;
        }

        //if we got here it means we haven't found a free id,
        //so we need to add a new block
        idBitmap.push_back(0xFFFFFFFF - 1);

        return blockId * idsPerBlock;
    }

    void IdGenerator::freeId(uint32_t id) {
        auto blockId = id / idsPerBlock;
        auto bit = id % idsPerBlock;

        if (idBitmap.size() > blockId) {
            idBitmap[blockId] |= 1 << bit;
        }
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

                            auto task = currentScheduler->createUserTask(run.entryPoint, path);
                            currentScheduler->scheduleTask(task);

                            RunResult result;
                            result.recipientId = run.senderTaskId;
                            result.success = task != nullptr;
                            result.pid = task->id;
                            send(IPC::RecipientType::TaskId, &result);

                            break;
                        }
                    }
                }

                break;
            }
        }
    }

    Scheduler::Scheduler(CPU::TSS* kernelTSS) {
        currentScheduler = this;

        taskBuffer = new Task[30];
        kernelHeap = LibC_Implementation::KernelHeap;
        kernelVMM = Memory::currentVMM;
        this->kernelTSS = kernelTSS;

        startTask = createKernelTask(reinterpret_cast<uint32_t>(idleLoop));
        cleanupTask = createKernelTask(reinterpret_cast<uint32_t>(cleanupTasksService));
        cleanupTask->state = TaskState::Blocked;
        schedulerTask = createKernelTask(reinterpret_cast<uint32_t>(schedulerService));
        schedulerTask->state = TaskState::Blocked;
        blockedQueue.append(cleanupTask);
        blockedQueue.append(schedulerTask);

        elapsedTime_milliseconds = 0;
        timeslice_milliseconds = 10;
    }

    void Scheduler::cleanupTasks() {
        while (true) {
            asm volatile("cli");
            auto task = deleteQueue.getHead();

            while (task != nullptr) {
                auto kernelStackAddress = (task->context.kernelESP & ~0xFFF);
                delete reinterpret_cast<Stack*>(kernelStackAddress);
                auto next = task->nextTask;
                deleteQueue.remove(task);
                task = next;
            }

            asm volatile("sti");
            blockTask(BlockReason::WaitingForMessage, 0);
        }
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
            if (startTask == currentTask) {
                return;
            }

            changeProcess(startTask, currentTask);
        }
        else if (state == State::ChangeToStart) {
            auto current = currentTask;
            currentTask = nullptr;
            if (current == startTask) {
                return;
            }

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

        if (!startedTasks) {
            return;
        }

        elapsedTime_milliseconds += timeslice_milliseconds;

        unblockWakeableTasks();
        scheduleNextTask(); 
        runNextTask();
    }

    /*
    When creating a user task, we need to push additional arguments to the stack
    that launchProcess uses to run the actual user code. So we need to move
    the kernel stack up a bit to fit those arguments at the bottom of the stack.
    */
    uint8_t volatile* adjustStack(Task* task, uint32_t offset) {
        auto address = task->context.esp;
        uint8_t volatile* stackPointer = static_cast<uint8_t volatile*>
            (reinterpret_cast<void volatile*>(address));

        InitialKernelStack volatile* stack = reinterpret_cast<InitialKernelStack volatile*>(stackPointer);

        auto eip = stack->eip;
        
        for(auto i = 0u; i < sizeof(InitialKernelStack); i++) {
            *stackPointer = 0;
            stackPointer++;
        }

        stackPointer -= sizeof(InitialKernelStack);
        stackPointer -= offset;
        stack = reinterpret_cast<InitialKernelStack volatile*>(stackPointer);

        stack->eip = eip;

        task->context.esp = reinterpret_cast<uint32_t>(stackPointer);

        return stackPointer + sizeof(InitialKernelStack);
    }
    
    Task* Scheduler::createKernelTask(uintptr_t functionAddress) {
        auto oldHeap = LibC_Implementation::KernelHeap;
        LibC_Implementation::KernelHeap = kernelHeap;
        auto oldVMM = Memory::currentVMM;
        kernelVMM->activate();

        uint32_t stackAddress {0};
        {
            auto stack = new Stack;
            memset(stack->data, 0, sizeof(Stack));
            stackAddress = reinterpret_cast<uint32_t>(stack);
        }

        uint8_t volatile* stackPointer = static_cast<uint8_t volatile*>
            (reinterpret_cast<void volatile*>(stackAddress + sizeof(Stack)));
        stackPointer -= sizeof(InitialKernelStack);

        InitialKernelStack volatile* stack = reinterpret_cast<InitialKernelStack volatile*>(stackPointer);
        
        stack->eip = functionAddress;

        Task* task = taskBuffer;
        task->id = idGenerator.generateId();
        task->context.esp = reinterpret_cast<uint32_t>(stackPointer);
        task->context.kernelESP = task->context.esp;
        task->virtualMemoryManager = kernelVMM;
        task->heap = kernelHeap;
        task->tss = kernelTSS;

        const uint32_t mailboxSize = 2 * Memory::PageSize;
        auto mailboxMemory = task->heap->aligned_allocate(Memory::PageSize, mailboxSize);

        task->mailbox = new (mailboxMemory) 
            IPC::Mailbox(reinterpret_cast<uint32_t>(mailboxMemory) + sizeof(IPC::Mailbox), 
            mailboxSize - sizeof(IPC::Mailbox));

        auto kernelStackPointer = adjustStack(task, 4);
        task->context.kernelESP = task->context.esp;
        auto stackExtras = reinterpret_cast<uint32_t volatile*>(kernelStackPointer);
        *stackExtras = reinterpret_cast<uint32_t>(callExitKernelTask);

        taskBuffer++;

        oldVMM->activate();
        LibC_Implementation::KernelHeap = oldHeap;

        return task;
    }

    Task* Scheduler::createUserTask(uintptr_t functionAddress, char* path) {
        auto oldHeap = LibC_Implementation::KernelHeap;
        LibC_Implementation::KernelHeap = kernelHeap;
        auto oldVMM = Memory::currentVMM;
        kernelVMM->activate();

        auto task = createKernelTask(reinterpret_cast<uintptr_t>(launchProcess));
        auto vmm = kernelVMM->cloneForUsermode();
        task->virtualMemoryManager = vmm;
        auto backupTSS = *kernelTSS;
        vmm->activate();
        vmm->HACK_setNextAddress(0xcfff'f000);
        auto tssAddress = vmm->allocatePages(1, static_cast<int>(Memory::PageTableFlags::AllowWrite));
        task->tss = reinterpret_cast<CPU::TSS*>(tssAddress);
        *task->tss = backupTSS;

        for (auto i = 0u; i < sizeof(CPU::TSS::ioPermissionBitmap); i++) {
            task->tss->ioPermissionBitmap[i] = 0xFF;
        }

        if (vmm != Memory::currentVMM) {
            kprintf("[Scheduler] Error: vmm->activate failed\n");
            asm volatile("hlt");
        }

        vmm->HACK_setNextAddress(0xa000'0000);
        auto userStackAddress = vmm->allocatePages(sizeof(Stack) / Memory::PageSize, static_cast<int>(Memory::PageTableFlags::AllowWrite));
        auto stack = reinterpret_cast<volatile Stack*>(userStackAddress);
        memset(const_cast<Stack*>(stack)->data, 0, sizeof(Stack));

        LibC_Implementation::createHeap(Memory::PageSize * Memory::PageSize, vmm);
        task->heap = LibC_Implementation::KernelHeap;

        /*
        Need to adjust the kernel stack because we want to add
        userESP and userEIP to the top
        */
        auto spaceToAdjust = 8;

        if (path != nullptr) {
            spaceToAdjust += 4;
        }

        auto kernelStackPointer = adjustStack(task, spaceToAdjust);
        task->context.kernelESP = task->context.esp;

        uint8_t volatile* userStackPointer = static_cast<uint8_t volatile*>
            (reinterpret_cast<void volatile*>(userStackAddress + sizeof(Stack)));//4096));
        userStackPointer -= sizeof(TaskStack);
        TaskStack volatile* userStack = reinterpret_cast<TaskStack volatile*>
            (userStackPointer);

        userStack->eflags = reinterpret_cast<TaskStack volatile*>
            (kernelStackPointer - sizeof(TaskStack))->eflags;
        userStack->eip = functionAddress;

        if (path != nullptr) {
            char* pathCopy = new char[strlen(path) + 1];
            memset(pathCopy, '\0', strlen(path) + 1);
            memcpy(pathCopy, path, strlen(path));
            userStack->edi = reinterpret_cast<uintptr_t>(pathCopy);
        }
        
        /*
        createUserTask creates a task that starts inside launchProcess,
        which pops off two values off the stack (user ESP and user EIP)
        and then enters the user task. So we need to write those two values
        here
        */
        auto stackExtras = reinterpret_cast<uint32_t volatile*>(kernelStackPointer);
        *stackExtras++ = reinterpret_cast<uint32_t>(vmm);
        *stackExtras++ = reinterpret_cast<uint32_t>(userStackPointer);

        if (path == nullptr) {
            *stackExtras++ = reinterpret_cast<uint32_t>(usermodeStub);
        }
        else {
            *stackExtras++ = reinterpret_cast<uint32_t>(elfUsermodeStub);
        }

        oldVMM->activate();
        LibC_Implementation::KernelHeap = oldHeap;

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
                    if (ServiceRegistryInstance->isPseudoService(message->serviceType)) {
                        ServiceRegistryInstance->receivePseudoMessage(message->serviceType, message);
                        return;
                    }
                    else {
                        kprintf("[Scheduler] Unknown Service Name\n");
                        return;
                    }
                }
            }
            else if (recipient == IPC::RecipientType::Scheduler) {
                taskId = schedulerTask->id;
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

            if (!readyQueue.isEmpty()) {
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

        kprintf("[Scheduler] Unsent message from: %d to: %d type: %d\n", message->senderTaskId, taskId, recipient);
        asm("hlt");
    }

    void Scheduler::receiveMessage(IPC::Message* buffer) {
        if (currentTask != nullptr) {
            auto task = currentTask;

            if (!currentTask->mailbox->hasUnreadMessages()) {
                blockTask(BlockReason::WaitingForMessage, 0);
            }

            task->mailbox->receive(buffer);
        }
    }

    void Scheduler::receiveMessage(IPC::Message* buffer, IPC::MessageNamespace filter, uint32_t messageId) {
        if (currentTask != nullptr) {
            auto task = currentTask;

            while (true) {

                if (!currentTask->mailbox->hasUnreadMessages()) {
                    blockTask(BlockReason::WaitingForMessage, 0);
                }

                task->mailbox->receive(buffer);

                if (buffer->messageNamespace != filter
                        || buffer->messageId != messageId) {

                    task->mailbox->send(buffer);
                    blockTask(BlockReason::WaitingForMessage, 0);
                }
                else {
                    return;
                }
            }
        }
    }

    bool Scheduler::peekReceiveMessage(IPC::Message* buffer) {
        if (currentTask != nullptr) {
            auto task = currentTask;

            if (!currentTask->mailbox->hasUnreadMessages()) {
                return false;
            }

            task->mailbox->receive(buffer);
            return true;
        }

        return false;
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
       if (false) { 
        auto kernelStackAddress = (currentTask->context.esp & ~0xFFF) + Memory::PageSize;
        auto kernelStack = reinterpret_cast<uint32_t volatile*>(kernelStackAddress);
        auto userStackAddress = *(kernelStack - 2) & ~0xFFF;
        auto userStack = reinterpret_cast<Stack*>(userStackAddress);

        delete userStack;
        //currentTask->virtualMemoryManager->freePages(reinterpret_cast<uint32_t>(currentTask->heap), 1);
        auto pageDirectory = currentTask->virtualMemoryManager->getPageDirectory();
        currentTask->virtualMemoryManager->cleanup();
        kernelVMM->activate();
        LibC_Implementation::KernelHeap = kernelHeap;
        kernelVMM->cleanupClonePageTables(pageDirectory, kernelStackAddress - Memory::PageSize);
        delete currentTask->virtualMemoryManager;
        kernelHeap->free(currentTask->mailbox);
       }

        idGenerator.freeId(currentTask->id);

        currentTask->state = TaskState::Blocked;
        scheduleNextTask();
        readyQueue.remove(currentTask);
        deleteQueue.append(currentTask);
        unblockTask(cleanupTask);
        
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
        kernelVMM->activate();
        LibC_Implementation::KernelHeap = kernelHeap;
        kernelHeap->free(currentTask->mailbox);

        idGenerator.freeId(currentTask->id);

        currentTask->state = TaskState::Blocked;
        scheduleNextTask();
        readyQueue.remove(currentTask);
        deleteQueue.append(currentTask);
        unblockTask(cleanupTask);
        runNextTask();
    }

    void Scheduler::enterIdle() {
        startedTasks = true;
        startProcess(startTask);
    }
}