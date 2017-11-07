#pragma once

#include <task_context.h>

namespace Kernel {
    struct Task {
        TaskContext context;
        Task* nextTask {nullptr};
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