#pragma once

#include <task_context.h>

namespace Kernel {
    struct Task {
        TaskContext context;
        Task* nextTask {nullptr};
        uint32_t id {0};
    };

    enum class EFlags {
        Reserved = 1 << 1,
        InterruptEnable = 1 << 9
    };

    struct TaskStack {
        uint32_t eflags;
        uint32_t edi {0};
        uint32_t esi {0};
        uint32_t ebp {0};
        uint32_t ebx {0};
        uint32_t edx {0};
        uint32_t ecx {0};
        uint32_t eax {0};

        /*uint32_t ss;
        uint32_t esp;
        uint32_t eflags2;
        uint32_t cs {0};
        */
        uint32_t eip {0};

        /*uint32_t eip;
        uint32_t cs;
        uint32_t eflags2;
        uint32_t esp;
        uint32_t ss;*/
    };

    extern class Scheduler* currentScheduler;

    class Scheduler {
    public:

        Scheduler();
        
        void notifyTimesliceExpired();
        Task* createTestTask(uintptr_t functionAddress);
        void scheduleTask(Task* task);
        void enterIdle();

    private:

        Task idleTask;
        //TODO: HACK: need to make a proper kernel allocator for this
        Task* taskBuffer;
        Task* currentTask;
        Task* startTask;
    };
}