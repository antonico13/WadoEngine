#ifndef WADO_TASK_SYSTEM_H
#define WADO_TASK_SYSTEM_H

#include "FiberSystem.h"
#include "Thread.h"
#include "Fiber.h"
#include "LockFreeFIFO.h"

namespace Wado::Task {
    extern Wado::Thread::WdThreadLocalID TLlocalReadyQueueID;
    extern Wado::Thread::WdThreadLocalID TLpreviousFiberID;
    extern Wado::Thread::WdThreadLocalID TLallocatorID;
    extern Wado::Fiber::WdFiberLocalID FLqueueNodeID;

    extern Wado::Queue::LockFreeQueue<void> FiberGlobalReadyQueue;

    class Fence;

    // two pointer derefs with this, 
    // will hit really bad performance
    using WdFiberArgs = struct WdFiberArgs {
        void *mainArgs; 
        Fence *fenceToSignal; 
        Wado::FiberSystem::WdReadyQueueItem readyQueueItem;
    };

    #define TASK(TaskName, ArgumentType, ArgumentName, Code)\
        void TaskName(void *fiberParam) {\
            Wado::Fiber::WdFiberID previousFiberID = static_cast<Wado::Fiber::WdFiberID>(Wado::Thread::WdThreadLocalGetValue(TLpreviousFiberID));\
            if (previousFiberID != nullptr) {\
                Wado::Fiber::WdDeleteFiber(previousFiberID);\
            };\
            WdFiberArgs *args = static_cast<WdFiberArgs *>(fiberParam);\
            Wado::Fiber::WdFiberLocalSetValue(FLqueueNodeID, args->readyQueueItem);\
            ArgumentType *ArgumentName = static_cast<ArgumentType *>(args->mainArgs);\
            Code;\
            if (args->fenceToSignal != nullptr) {\
                args->fenceToSignal->signal();\
            };\
            Wado::Fiber::WdFiberID nextFiberID = Wado::FiberSystem::PickNewFiber()->data;\
            Wado::Thread::WdThreadLocalSetValue(TLpreviousFiberID, Wado::Fiber::WdGetCurrentFiber());\
            Wado::FiberSystem::WdReadyQueueItem readyQueueItem = static_cast<Wado::FiberSystem::WdReadyQueueItem>(Wado::Fiber::WdFiberLocalGetValue(FLqueueNodeID));\
            FiberGlobalReadyQueue.release(readyQueueItem);\
            free(readyQueueItem);\
            free(args);\
            Wado::Fiber::WdSwitchFiber(nextFiberID);\
        };

    void makeTask(Wado::Fiber::WdFiberFunctionPtr functionPtr, void *arguments, Fence* fenceToSignal = nullptr);
};

#endif