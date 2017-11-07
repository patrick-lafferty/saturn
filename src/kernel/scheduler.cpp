#include "scheduler.h"
#include <stdio.h>
#include <memory/physical_memory_manager.h>
#include <memory/virtual_memory_manager.h>

extern "C" void changeProcess(Kernel::Task* current, Kernel::Task* next);

namespace Kernel {

    Scheduler* currentScheduler;

    void idleLoop() {
        printf("[Scheduler] Idle\n");
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
    }

    void Scheduler::notifyTimesliceExpired() {

        auto current = currentTask;
        auto next = currentTask->nextTask;

        if (currentTask->nextTask == nullptr) {
            next = startTask;
            currentTask = startTask;
        }
        else {
            currentTask = currentTask->nextTask;
        }

        changeProcess(current, next);
    }

    Task* Scheduler::createTestTask(uintptr_t functionAddress) {
        auto processStack = Memory::currentVMM->allocatePages(1, static_cast<int>(Memory::PageTableFlags::Present)
            | static_cast<int>(Memory::PageTableFlags::AllowWrite));
        auto physicalPage = Memory::currentPMM->allocatePage(1);
        Memory::currentVMM->map(processStack, physicalPage);
        Memory::currentPMM->finishAllocation(processStack, 1);

        uint32_t volatile* stackPointer = static_cast<uint32_t volatile*>(reinterpret_cast<void volatile*>(processStack + 4096));
        *(--stackPointer) = functionAddress; 
        *(--stackPointer) = 0; //eax
        *(--stackPointer) = 0; //ecx
        *(--stackPointer) = 0; //edx
        *(--stackPointer) = 0; //ebx
        *(--stackPointer) = 0; //ebp
        *(--stackPointer) = 0; //esi
        *(--stackPointer) = 0; //edi
        *(--stackPointer) = 1 << 9 | 1 << 1; 

        Task* task = taskBuffer;
        task->context.esp = reinterpret_cast<uint32_t>(stackPointer);

        taskBuffer++;

        return task;
    }

    void Scheduler::scheduleTask(Task* task) {

        auto current = currentTask;

        while (current->nextTask != nullptr) {
            current = current->nextTask;
        }

        current->nextTask = task;
    }

    void Scheduler::enterIdle() {
        idleLoop();
    }
}