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
#pragma once

#include <stdint.h>
#include <task_context.h>
#include <vector>
#include <cpu/tss.h>
#include <memory/block_allocator.h>
#include <memory/virtual_memory_manager.h>
#include "ipc.h"

namespace LibC_Implementation {
    class Heap;
}

namespace Kernel {

    enum class TaskState {
        Running,
        Sleeping,
        Blocked
    };

    struct InitialKernelStack {
        uint32_t eip {0};
    };

    struct alignas(0x1000) Stack {
        char data[0x100000];
    };

    struct TaskStack {
        uint32_t eflags;
        uint32_t edi {0};
        uint32_t esi {0};
        uint32_t ebp {0};
        uint32_t ebx {0};
        uint32_t eip {0};
    };

    struct alignas(16) SSEContext {
        uint8_t data[512];
    };

    struct KernelStackExtras {
        uintptr_t exitAddress;
    };

    struct UserStackExtras {
        uintptr_t usermodeStubAddress;
        uintptr_t userStackAddress;
        uintptr_t vmmAddress;
    };

    enum class Priority {
        IRQ,
        Input,
        UI,
        IO,
        Other,
        Idle,
        Last
    };

    /*
    A Task is a running application or service, which may be
    user-mode or kernel-mode
    */
    struct Task {
        TaskContext context;
        SSEContext* sseContext;
        Task* nextTask {nullptr};
        Task* previousTask {nullptr};
        uint32_t id {0};
        TaskState state;
        uint64_t wakeTime {0};
        Memory::VirtualMemoryManager* virtualMemoryManager;
        LibC_Implementation::Heap* heap;
        IPC::Mailbox* mailbox;
        CPU::TSS* tss;
        Stack* kernelStack;
        Priority priority {Priority::Other};
    };

    /*
    Tracks available task ids and allows them to be recycled
    when a task exits
    */
    class IdGenerator {
    public:

        IdGenerator();

        uint32_t generateId();
        void freeId(uint32_t id);

    private:

        uint32_t idBitmap[64];
        const uint32_t idsPerBlock {32};
    };

    struct BufferedMailbox {
        IPC::Mailbox box;
        static constexpr int bufferSize = 20 * 0x1000;
        uint8_t buffer[bufferSize];

        BufferedMailbox() 
            : box {IPC::Mailbox {reinterpret_cast<uintptr_t>(this) + sizeof(box), 
                    bufferSize}} {
        }
    };

    class TaskLauncher {
    public:

        TaskLauncher(CPU::TSS* kernelTSS);

        Task* createKernelTask(uintptr_t functionAddress);
        Task* createUserTask(uintptr_t functionAddress);

        void freeKernelTask(Task* task);
        void freeUserTask(Task* task);

    private:

        CPU::TSS* kernelTSS;
        Memory::VirtualMemoryManager* kernelVMM;
        LibC_Implementation::Heap* kernelHeap;
        IdGenerator idGenerator;

        BlockAllocator<Stack> stackAllocator;
        BlockAllocator<Task> taskAllocator;
        BlockAllocator<BufferedMailbox> mailboxAllocator;
        BlockAllocator<Memory::VirtualMemoryManager> vmmAllocator;
    };

    inline TaskLauncher* CurrentTaskLauncher;
    
}
