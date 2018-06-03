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
#include <heap.h>
#include <cpu/cpu.h>

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
        if (offset > 4) offset -= 4;
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
        : stackAllocator {Memory::currentVMM},
            taskAllocator {Memory::currentVMM},
            mailboxAllocator {Memory::currentVMM},
            vmmAllocator {Memory::currentVMM},
            kernelTSS {kernelTSS}
        {
        kernelHeap = LibC_Implementation::KernelHeap;
        kernelVMM = Memory::currentVMM;
        CurrentTaskLauncher = this;
    }

    uint8_t volatile* getStackPointer(Stack* stack) {
        auto stackAddress = reinterpret_cast<uintptr_t>(stack);

        uint8_t volatile* stackPointer = static_cast<uint8_t volatile*>
            (reinterpret_cast<void volatile*>(stackAddress + sizeof(Stack)));
        stackPointer -= sizeof(InitialKernelStack);

        return stackPointer;
    }

    Task* TaskLauncher::createKernelTask(uintptr_t functionAddress) {

        MemoryGuard guard {kernelVMM, kernelHeap};

        auto kernelStack = stackAllocator.allocate(); 
        auto stackPointer = getStackPointer(kernelStack);

        InitialKernelStack volatile* stack = reinterpret_cast<InitialKernelStack volatile*>(stackPointer);
        stack->eip = functionAddress;

        Task* task = taskAllocator.allocate();
        task->id = idGenerator.generateId();
        task->context.esp = reinterpret_cast<uintptr_t>(stackPointer);
        task->context.kernelESP = task->context.esp;
        task->virtualMemoryManager = kernelVMM;
        task->heap = kernelHeap;
        task->tss = kernelTSS;
        task->kernelStack = kernelStack;
        task->mailbox = &mailboxAllocator.allocate()->box;

        adjustStack<KernelStackExtras>(task, {reinterpret_cast<uintptr_t>(callExitKernelTask)});

        //auto sseMemory = task->heap->aligned_allocate(16, sizeof(SSEContext));
        //task->sseContext = new (sseMemory) SSEContext;

        return task;
    }

    Task* TaskLauncher::createUserTask(uintptr_t functionAddress) {

        MemoryGuard guard {kernelVMM, kernelHeap};

        auto task = createKernelTask(reinterpret_cast<uintptr_t>(launchProcess));
        auto vmm = vmmAllocator.allocate();
        kernelVMM->cloneForUsermode(vmm);
        task->virtualMemoryManager = vmm;
        auto backupTSS = *kernelTSS;
        vmm->activate();
        BlockAllocator<CPU::TSS> tssAllocator {vmm, 1};
        task->tss = tssAllocator.allocateFrom(0xcfff'f000);
        *task->tss = backupTSS;

        for (auto i = 0u; i < sizeof(CPU::TSS::ioPermissionBitmap); i++) {
            task->tss->ioPermissionBitmap[i] = 0xFF;
        }

        BlockAllocator<Stack> userStackAllocator {vmm, 1};
        auto stack = userStackAllocator.allocateFrom(0xa000'0000);

        LibC_Implementation::createHeap(Memory::PageSize * Memory::PageSize, vmm);
        task->heap = LibC_Implementation::KernelHeap;

        //auto sseMemory = task->heap->aligned_allocate(16, sizeof(SSEContext));
        //task->sseContext = new (sseMemory) SSEContext;
        auto userStackPointer = getStackPointer(stack);
        userStackPointer -= sizeof(TaskStack);

        adjustStack<UserStackExtras>(task, UserStackExtras {
            reinterpret_cast<uintptr_t>(vmm), 
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
}