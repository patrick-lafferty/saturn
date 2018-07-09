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
#include "task.h"
#include <memory/guard.h>
#include <memory/physical_memory_manager.h>
#include <cpu/cpu.h>
#include <locks.h>

#include <saturn/heap.h>

extern "C" void launchProcess();
extern "C" void usermodeStub();
extern "C" void elfUsermodeStub();

namespace Kernel {

    /*
    When creating a user task, we need to push additional arguments to the stack
    that launchProcess uses to run the actual user code. So we need to move
    the kernel stack up a bit to fit those arguments at the bottom of the stack.
    */
    template<typename T>
    void adjustStack(Task* task, T extra) {

        auto offset = sizeof(T);
        auto address = task->context.esp;
        auto stackPointer = static_cast<uint8_t*>(reinterpret_cast<void*>(address));
        auto stack = reinterpret_cast<InitialKernelStack*>(stackPointer);
        auto eip = stack->eip;
        
        for(auto i = 0u; i < sizeof(InitialKernelStack); i++) {
            *stackPointer = 0;
            stackPointer++;
        }

        stackPointer -= sizeof(InitialKernelStack);
        stackPointer -= offset;
        stack = reinterpret_cast<InitialKernelStack*>(stackPointer);
        stack->eip = eip;

        task->context.esp = reinterpret_cast<uint32_t>(stackPointer);
        task->context.kernelESP = task->context.esp;

        auto extras = reinterpret_cast<T*>(stackPointer + sizeof(InitialKernelStack));
        *extras = extra;
    }

    IdGenerator::IdGenerator() {
        for (auto& block : idBitmap) {
            block = 0xFFFFFFFF - 1;
        }
    }

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
        //idBitmap.push_back(0xFFFFFFFF - 1);
        //TODO

        return blockId * idsPerBlock;
    }

    void IdGenerator::freeId(uint32_t id) {
        auto blockId = id / idsPerBlock;
        auto bit = id % idsPerBlock;

        idBitmap[blockId] |= 1 << bit;
    }

    void callExitKernelTask() {
        CPU::exitKernelTask();
    }

    TaskLauncher::TaskLauncher(CPU::TSS* kernelTSS)
        : stackAllocator {Memory::InitialKernelVMM},
            smallStackAllocator {Memory::InitialKernelVMM},
            taskAllocator {Memory::InitialKernelVMM},
            mailboxAllocator {Memory::InitialKernelVMM},
            vmmAllocator {Memory::InitialKernelVMM},
            kernelTSS {kernelTSS}
        {
        kernelVMM = Memory::InitialKernelVMM;
        CurrentTaskLauncher = this;
    }

    template<typename StackType>
    uint8_t volatile* getStackPointer(StackType* stack) {
        auto stackAddress = reinterpret_cast<uintptr_t>(stack);

        uint8_t volatile* stackPointer = static_cast<uint8_t volatile*>
            (reinterpret_cast<void volatile*>(stackAddress + sizeof(StackType)));
        stackPointer -= sizeof(InitialKernelStack);

        return stackPointer;
    }

    Task* TaskLauncher::createKernelTask(uintptr_t functionAddress) {
        SpinLock spin {&lock};

        MemoryGuard guard {kernelVMM};

        auto kernelStack = smallStackAllocator.allocate();
        memset(kernelStack, 0, sizeof(SmallStack));
        auto stackPointer = getStackPointer(kernelStack);

        InitialKernelStack volatile* stack = reinterpret_cast<InitialKernelStack volatile*>(stackPointer);
        stack->eip = functionAddress;

        Task* task = taskAllocator.allocate();
        task->id = idGenerator.generateId();
        task->context.esp = reinterpret_cast<uintptr_t>(stackPointer);
        task->context.kernelESP = task->context.esp;
        task->virtualMemoryManager = kernelVMM;
        TaskStore::getInstance().storeTask(task);

        /*
        usermode tasks create their own heap, the kernel no longer has a heap, so kernel tasks
        need to make their own heap if they need one
        */
        if (functionAddress != reinterpret_cast<uintptr_t>(launchProcess)) {
            auto oldAddr = kernelVMM->HACK_getNextAddress();
            kernelVMM->HACK_setNextAddress(0xa000'0000 + 0x100000);
            task->heap = Saturn::Memory::createHeap(Memory::PageSize * Memory::PageSize, kernelVMM);
            kernelVMM->HACK_setNextAddress(oldAddr);
        }

        task->tss = kernelTSS;
        task->kernelStack = kernelStack;
        task->mailbox = &mailboxAllocator.allocate()->box;

        adjustStack<KernelStackExtras>(task, {reinterpret_cast<uintptr_t>(callExitKernelTask)});

        //auto sseMemory = task->heap->aligned_allocate(16, sizeof(SSEContext));
        //task->sseContext = new (sseMemory) SSEContext;

        return task;
    }

    Task* TaskLauncher::createUserTask(uintptr_t functionAddress) {

        MemoryGuard guard {kernelVMM};

        auto task = createKernelTask(reinterpret_cast<uintptr_t>(launchProcess));

        SpinLock spin {&lock};

        auto vmm = vmmAllocator.allocate();
        kernelVMM->cloneForUsermode(vmm);
        task->virtualMemoryManager = vmm;
        auto kernelTSS = reinterpret_cast<CPU::TSS*>(0xcfff'f000 - 0x1000 * CPU::getCurrentCPUId());
        auto backupTSS = *kernelTSS;
        vmm->activate();
        BlockAllocator<CPU::TSS> tssAllocator {vmm, 1};
        task->tss = tssAllocator.allocateFrom(0xcfff'f000 - 0x1000 * CPU::getCurrentCPUId());
        memset(task->tss, 0, sizeof(CPU::TSS));
        CPU::setupTSS(reinterpret_cast<uintptr_t>(task->tss));

        for (auto i = 0u; i < sizeof(CPU::TSS::ioPermissionBitmap); i++) {
            task->tss->ioPermissionBitmap[i] = 0xFF;
        }

        BlockAllocator<Stack> userStackAllocator {vmm, 1};
        auto stack = userStackAllocator.allocateFrom(0xa000'0000);

        task->heap = Saturn::Memory::createHeap(Memory::PageSize * Memory::PageSize, vmm);

        //auto sseMemory = task->heap->aligned_allocate(16, sizeof(SSEContext));
        //task->sseContext = new (sseMemory) SSEContext;
        auto userStackPointer = getStackPointer(stack) + sizeof(InitialKernelStack);
        userStackPointer -= sizeof(TaskStack);

        adjustStack<UserStackExtras>(task, UserStackExtras {
            reinterpret_cast<uintptr_t>(userStackPointer), 
            reinterpret_cast<uintptr_t>(usermodeStub)});

        task->context.kernelESP = task->context.esp;

        auto userStack = reinterpret_cast<TaskStack volatile*>(userStackPointer);
        userStack->eip = functionAddress;

        return task;
    }

    void TaskLauncher::freeKernelTask(Task* task) {
        //TODO
    }

    void TaskLauncher::freeUserTask(Task* task) {
        //TODO
    }

    TaskStore* TaskStore::instance = nullptr;

    TaskStore::TaskStore() 
        : blockAllocator {Memory::InitialKernelVMM, 1} {
        blocks[0] = blockAllocator.allocate();
        maxIds = TaskBlock::IdsPerBlock;

        if (instance == nullptr) {
            instance = this;
        }
    }

    TaskStore& TaskStore::getInstance() {
        return *instance;
    }

    Task* TaskStore::getTask(uint32_t id) {
        if (id >= maxIds) {
            return nullptr;
        }

        auto blockId = id / TaskBlock::IdsPerBlock;
        auto taskId = id % TaskBlock::IdsPerBlock;

        return blocks[blockId]->tasks[taskId];
    }

    void TaskStore::storeTask(Task* task) {
        auto id = task->id;

        if (id >= maxIds) {
            //panic();
        }

        auto blockId = id / TaskBlock::IdsPerBlock;
        auto taskId = id % TaskBlock::IdsPerBlock;

        blocks[blockId]->tasks[taskId] = task;
    }
}
