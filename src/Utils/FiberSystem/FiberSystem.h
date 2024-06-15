#ifndef WADO_FIBER_SYSTEM_H
#define WADO_FIBER_SYSTEM_H

#include "Queue.h"
#include <cstdint>

namespace Wado::FiberSystem {

    using WdReadyQueueItem = Wado::Queue::Queue<void>::Item*;
    
    WdReadyQueueItem PickNewFiber();
    void InitWorkerFiber();
    unsigned long WorkerThreadStartFunction(void *param);
    void FiberYield();
    void InitFiberSystem();
    void ShutdownFiberSystem();

    class WdLock {
        public:
            WdLock() noexcept;
            ~WdLock() noexcept;
            void acquire();
            void release();
        private:
            // TODO: these dont have to be longs, can do 16 too 
            const uint64_t _unlocked = 0;
            const uint64_t _locked = 1;

            uint64_t volatile _waitGuard;
            uint64_t volatile _willYield;
            uint64_t volatile _lock;
            Wado::Fiber::WdFiberID _ownerFiber;
            // This is kinda wild 
            Wado::Queue::LinkedListQueue<void> _waiters;
    };

    class WdFence {
        public:
            WdFence(size_t count) noexcept;
            ~WdFence() noexcept;
            void signal() noexcept;
            void waitForSignal() noexcept;
        private:
            const uint64_t _unsignaled = 0;
            const uint64_t _signaled = 1;
        
            uint64_t volatile fence;
            uint64_t volatile _waiterGuard;
            // This is kinda wild 
            Wado::Queue::LinkedListQueue<void> waiters;
    };
};

#endif