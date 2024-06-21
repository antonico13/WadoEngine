#ifndef WADO_TASK_SYSTEM_H
#define WADO_TASK_SYSTEM_H

#include "FiberSystem.h"
#include "Thread.h"
#include "Fiber.h"
#include "LockFreeFIFO.h"

extern Wado::Thread::WdThreadLocalID TLlocalReadyQueueID;
extern Wado::Thread::WdThreadLocalID TLpreviousFiberID;
extern Wado::Thread::WdThreadLocalID TLallocatorID;
extern Wado::Thread::WdThreadLocalID TLidleFiberID;
extern Wado::Thread::WdThreadLocalID TLcoreIndexID;
extern Wado::Fiber::WdFiberLocalID FLqueueNodeID;

extern Wado::Queue::LockFreeQueue<void> FiberGlobalReadyQueue;

namespace Wado::Task {
    // two pointer derefs with this, 
    // will hit really bad performance
    using WdFiberArgs = struct WdFiberArgs {
        void *mainArgs; 
        Wado::FiberSystem::WdFence *fenceToSignal; 
        Wado::FiberSystem::WdReadyQueueItem readyQueueItem;
    };

    #define TASK(TaskName, ArgumentType, ArgumentName, Code)\
        void TaskName(void *fiberParam) {\
            Wado::Task::WdFiberArgs *args = static_cast<Wado::Task::WdFiberArgs *>(fiberParam);\
            Wado::Fiber::WdFiberLocalSetValue(FLqueueNodeID, args->readyQueueItem);\
            ArgumentType *ArgumentName = static_cast<ArgumentType *>(args->mainArgs);\
            Code;\
            if (args->fenceToSignal != nullptr) {\
                args->fenceToSignal->signal();\
            };\
            Wado::Thread::WdThreadLocalSetValue(TLpreviousFiberID, Wado::Fiber::WdGetCurrentFiber());\
            Wado::FiberSystem::WdReadyQueueItem readyQueueItem = static_cast<Wado::FiberSystem::WdReadyQueueItem>(Wado::Fiber::WdFiberLocalGetValue(FLqueueNodeID));\
            FiberGlobalReadyQueue.release(readyQueueItem);\
            free(readyQueueItem);\
            free(args);\
            Wado::FiberSystem::FiberYield();\
        };

    void makeTask(Wado::Fiber::WdFiberFunctionPtr functionPtr, void *arguments, Wado::FiberSystem::WdFence* fenceToSignal = nullptr);
};

#endif