#include "Windows.h"

#include "FiberSystem.h"

#include "System.h"
#include "Thread.h"
#include "Fiber.h"

#include "Atomics.h"

#include "ArrayQueue.h"
#include "LinkedListQueue.h"
#include "LockFreeFIFO.h"

#include <stdexcept>

namespace Wado::FiberSystem {

    const size_t DEFAULT_LOCAL_READY_QUEUE_SIZE = 20;

    Wado::Thread::WdThreadLocalID TLlocalReadyQueueID;
    Wado::Thread::WdThreadLocalID TLpreviousFiberID;
    Wado::Thread::WdThreadLocalID TLallocatorID;
    Wado::Thread::WdThreadLocalID TLidleFiberID;
    Wado::Fiber::WdFiberLocalID FLqueueNodeID;

    Wado::Queue::LockFreeQueue<void> FiberGlobalReadyQueue;

    volatile uint64_t close = 0;

    void InitWorkerFiber() {
        LPVOID lpvQueue; 
        lpvQueue = (LPVOID) HeapAlloc(GetProcessHeap(), 0, sizeof(Wado::Queue::ArrayQueue<void>));

        if (lpvQueue == NULL) {
            throw std::runtime_error("Could not allocate memory for local queues for main thread");
        };

        new (lpvQueue) Wado::Queue::ArrayQueue<void>(DEFAULT_LOCAL_READY_QUEUE_SIZE);
        
        Wado::Thread::WdThreadLocalSetValue(TLlocalReadyQueueID, lpvQueue);

        // Convert worker thread to fiber 
        Wado::Fiber::WdFiberID currentFiberID = Wado::Fiber::WdStartFiber();
        
        Wado::Thread::WdThreadLocalSetValue(TLidleFiberID, currentFiberID);
    };

    void IdleFiberFunction(void *param) {
        Wado::Queue::Queue<void> *localReadyQueue = static_cast<Wado::Queue::Queue<void> *>(Wado::Thread::WdThreadLocalGetValue(TLlocalReadyQueueID));
        WdReadyQueueItem nextFiber = nullptr;

        while (close == 0) {
            if (nextFiber != nullptr) {
                Wado::Fiber::WdSwitchFiber(nextFiber->data);
                Wado::Fiber::WdFiberID previousFiberID = static_cast<Wado::Fiber::WdFiberID>(Wado::Thread::WdThreadLocalGetValue(TLpreviousFiberID));
                if (previousFiberID != nullptr) {
                    Wado::Fiber::WdDeleteFiber(previousFiberID);
                    Wado::Thread::WdThreadLocalSetValue(TLpreviousFiberID, nullptr);
                };
                nextFiber = nullptr;
            };

            if (!localReadyQueue->isEmpty()) {
                nextFiber = localReadyQueue->dequeue();
                continue;
            };

            nextFiber = FiberGlobalReadyQueue.dequeue();
        };

        //std::cout << "About to exit worker thread" << std::endl;
        //std::cout << "Exiting initial worker thread" << std::endl;
        Wado::Queue::ArrayQueue<void> *threadLocalReadyQueue = static_cast<Wado::Queue::ArrayQueue<void> *>(Wado::Thread::WdThreadLocalGetValue(TLlocalReadyQueueID));

        threadLocalReadyQueue->~ArrayQueue();
        // TODO: de-windows this 
        HeapFree(GetProcessHeap(), 0, threadLocalReadyQueue);
            
        Wado::Fiber::WdDeleteFiber(Wado::Fiber::WdGetCurrentFiber());        
    };

    unsigned long WorkerThreadStartFunction(void *param) {

        InitWorkerFiber();
        Wado::Thread::WdThreadLocalSetValue(TLidleFiberID, Wado::Fiber::WdGetCurrentFiber());
        IdleFiberFunction(param);

        return 0;
    };

    void InitFiberSystem() {
        // //std::cout << "Started: " << std::endl;
        // HANDLE currentProcess = GetCurrentProcess();
        // //std::cout << "Current process pseudo handle: " << currentProcess << std::endl;
        // DWORD_PTR processAffinityMask;
        // DWORD_PTR systemAffinityMask;
        // BOOL res;
        // res = GetProcessAffinityMask(currentProcess, &processAffinityMask, &systemAffinityMask);

        Wado::System::WdAvailableCoresMask availableCoreMask = Wado::System::WdGetAvailableSystemCores();

        //std::bitset<sizeof(Wado::System::WdAvailableCoresMask)> systemSet(availableCoreMask);

        //std::cout << "Affinity mask get results: " << res << std::endl;
        //std::cout << "System affinity mask: " << availableCoreMask << std::endl;

        // Allocate TLS indices for thread local stuff 
        
        TLlocalReadyQueueID = Wado::Thread::WdThreadLocalAllocate();

        TLpreviousFiberID = Wado::Thread::WdThreadLocalAllocate();
        
        TLallocatorID = Wado::Thread::WdThreadLocalAllocate();

        TLidleFiberID = Wado::Thread::WdThreadLocalAllocate();

        FLqueueNodeID = Wado::Fiber::WdFiberLocalAllocate();

        const size_t lastBitMask = 1;

        size_t threadCount = 0;
        size_t coreIndex = 0;

        // TODO: this is a bit weird 
        size_t threadCoreAffinities[sizeof(Wado::System::WdAvailableCoresMask)];
        Wado::Thread::WdThreadHandle threadHandles[sizeof(Wado::System::WdAvailableCoresMask)];

        while (availableCoreMask > 0) {
            if (availableCoreMask & lastBitMask) {
                // This core bit is set, so we can assign a thread to it.
                threadCoreAffinities[threadCount] = coreIndex;
                threadCount++;
            };
            coreIndex++;
            availableCoreMask = availableCoreMask >> 1;
        };

        //std::cout << "Total core count: " << threadCount << std::endl;

        // Allocate memory for containers

        // TODO: need to de-windows this

        Wado::Thread::WdThreadHandle currrentThreadHandle = Wado::Thread::WdGetCurrentThreadHandle();
        threadHandles[0] = currrentThreadHandle;

        // Set current thread affinity 

        Wado::Thread::WdSetThreadAffinity(currrentThreadHandle, threadCoreAffinities[0]);

        // The current thread takes the first slot, now do all other threads 
        for (int i = 1; i < threadCount; ++i) {

            threadHandles[i] = Wado::Thread::WdCreateThread(WorkerThreadStartFunction, nullptr);
            
            if (threadHandles[i] == nullptr)  {
                throw std::runtime_error("Could not create worker threads");
            };

            Wado::Thread::WdSetThreadAffinity(threadHandles[i], threadCoreAffinities[i]);
        };

        InitWorkerFiber();

        Wado::Fiber::WdFiberID idleFiberID = Wado::Fiber::WdCreateFiber(IdleFiberFunction, nullptr);
        Wado::Thread::WdThreadLocalSetValue(TLidleFiberID, idleFiberID);

        // Start all other worker fibers 
        for (int i = 1; i < threadCount; i++) {
            Wado::Thread::WdStartThread(threadHandles[i]);
        };
    }; 

    void ShutdownFiberSystem() { 
        // Need to destroy all queue items etc here and send termination to all idle fibers 

        Wado::Atomics::TestAndSet(&close, 1);

        // TODO: need to wait on all worker threads here somehow 
        
        // int core = (int) TlsGetValue(coreNumberTlsIndex);
        // //std::cout << "Waiting on " << core << " for threads to finish" << std::endl;
        // for (int i = 0; i < threadCount; i++) {
        //     if (i != core) {
        //         WaitForSingleObject(threadHandleArray[i], INFINITE);
        //     };
        // };

        // //std::cout << "Finished waiting and closing everything" << std::endl;

        // for (int i = 0; i < threadCount; i++) {
        //     if (i != core) {
        //         CloseHandle(threadHandleArray[i]);
        //     };
        // };

        Wado::Fiber::WdFiberID currentIdleFiberID = Wado::Thread::WdThreadLocalGetValue(TLidleFiberID);
        
        Wado::Fiber::WdDeleteFiber(currentIdleFiberID);

        WdReadyQueueItem readyQueueItem = static_cast<WdReadyQueueItem>(Wado::Fiber::WdFiberLocalGetValue(FLqueueNodeID));
        FiberGlobalReadyQueue.release(readyQueueItem);
        free(readyQueueItem);

        Wado::Queue::ArrayQueue<void> *threadLocalReadyQueue = static_cast<Wado::Queue::ArrayQueue<void> *>(Wado::Thread::WdThreadLocalGetValue(TLlocalReadyQueueID));

        threadLocalReadyQueue->~ArrayQueue();
        // TODO: de-windows this 
        HeapFree(GetProcessHeap(), 0, threadLocalReadyQueue);

        FiberGlobalReadyQueue.~LockFreeQueue();

        Wado::Fiber::WdFiberLocalFree(FLqueueNodeID);

        Wado::Thread::WdThreadLocalFree(TLidleFiberID);
        Wado::Thread::WdThreadLocalFree(TLpreviousFiberID);
        Wado::Thread::WdThreadLocalFree(TLallocatorID);
        Wado::Thread::WdThreadLocalFree(TLlocalReadyQueueID);

        DeleteFiber(GetCurrentFiber());
    };

    void FiberYield() {
        Wado::Fiber::WdSwitchFiber(Wado::Thread::WdThreadLocalGetValue(TLidleFiberID)); 
    };

    // Synchronisation primitives 

    WdLock::WdLock() noexcept {
        _lock = _unlocked;
        _waitGuard = _unlocked;
        _willYield = _unlocked;
        _ownerFiber = nullptr;
    };

    WdLock::~WdLock() noexcept {
        // TODO: good?
        _waiters.~LinkedListQueue<void>();
    };

    void WdLock::acquire() {
        while (Wado::Atomics::TestAndSet(&_waitGuard, _locked) == _locked) { }; //spin

        if (_lock == _locked) {
            _waiters.enqueue(static_cast<WdReadyQueueItem>(Wado::Fiber::WdFiberLocalGetValue(FLqueueNodeID)));    
            // Need to say will yield here 
            _waitGuard = _unlocked;   
            //if (_willYield == _locked) {
            FiberYield();
            //};
        } else {
            _lock = _locked;
            _waitGuard = _unlocked;   
        };

        _ownerFiber = Wado::Fiber::WdGetCurrentFiber();
    };

    void WdLock::release() {
        if (_ownerFiber != Wado::Fiber::WdGetCurrentFiber()) {
            throw std::runtime_error("Tried to release a lock the fiber didn't onw");
        };
        
        while (Wado::Atomics::TestAndSet(&_waitGuard, _locked) == _locked) { }; // spin

        if (_waiters.isEmpty()) {
            _lock = _unlocked;
        } else {
            Wado::Queue::Queue<void> *localReadyQueue = static_cast<Wado::Queue::Queue<void> *>(Wado::Thread::WdThreadLocalGetValue(TLlocalReadyQueueID));
            WdReadyQueueItem nextFiber = _waiters.dequeue();
            // if (_willYield == _unlocked) {
            // if (localReadyQueue->isFull()) {
            FiberGlobalReadyQueue.enqueue(nextFiber);
            // } else {
            //     localReadyQueue->enqueue(nextFiber); 
            // };
        };
        _ownerFiber = nullptr;
        _waitGuard = _unlocked;
    };

    WdFence::WdFence(size_t count) noexcept {
        fence = count;
        _waiterGuard = _unsignaled;
    };

    WdFence::~WdFence() noexcept {

    };

    void WdFence::signal() noexcept {
        Wado::Atomics::Decrement(&fence);

        while (Wado::Atomics::TestAndSet(&_waiterGuard, _signaled) == _signaled) { }; //busy spin

        // std::cout << "Signaled fence " << std::endl;

        if (fence == 0) {
            // std::cout << "Reached count down" << std::endl;
            Wado::Queue::Queue<void> *localReadyQueue = static_cast<Wado::Queue::Queue<void> *>(Wado::Thread::WdThreadLocalGetValue(TLlocalReadyQueueID));
            
            while (!waiters.isEmpty()) {
                WdReadyQueueItem toWakeFiber = waiters.dequeue();
                //if (localReadyQueue->isFull()) {
                FiberGlobalReadyQueue.enqueue(toWakeFiber);
                // } else {
                //     localReadyQueue->enqueue(toWakeFiber); 
                // };
                }; 
        };
        _waiterGuard = _unsignaled;
    };

    void WdFence::waitForSignal() {
        if (fence > 0) {
            while (Wado::Atomics::TestAndSet(&_waiterGuard, _signaled) == _signaled) { }; // busy spin
            
            waiters.enqueue(static_cast<WdReadyQueueItem>(Wado::Fiber::WdFiberLocalGetValue(FLqueueNodeID))); 
            //std::cout << "About to yield for fence " << std::endl;
            // TODO: willYield here too 
            _waiterGuard = _unsignaled;
            FiberYield();
        };
    };
};