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
#include <bitset>

#include <string>

#ifndef NDEBUG
#include "DebugLog.h"
#include <cassert>
#include <iostream>
#endif

Wado::Thread::WdThreadLocalID TLlocalReadyQueueID;
Wado::Thread::WdThreadLocalID TLpreviousFiberID;
Wado::Thread::WdThreadLocalID TLallocatorID;
Wado::Thread::WdThreadLocalID TLidleFiberID;
Wado::Thread::WdThreadLocalID TLcoreIndexID;
Wado::Fiber::WdFiberLocalID FLqueueNodeID;

Wado::Queue::LockFreeQueue<void> FiberGlobalReadyQueue(8);

namespace Wado::FiberSystem {

    static Wado::Thread::WdThreadHandle threadHandles[sizeof(uint64_t) * 8];
    static size_t threadCount = 0;
    static volatile uint64_t close = 0;

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
        ////std::cout << "Worker fiber ID: " << currentFiberID << std::endl;
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

            if (!FiberGlobalReadyQueue.isEmpty()) {
                nextFiber = FiberGlobalReadyQueue.dequeue(); 
            }
        };

        ////std::cout << "About to exit worker thread" << std::endl;
        ////std::cout << "Exiting initial worker thread" << std::endl;
        Wado::Queue::ArrayQueue<void> *threadLocalReadyQueue = static_cast<Wado::Queue::ArrayQueue<void> *>(Wado::Thread::WdThreadLocalGetValue(TLlocalReadyQueueID));

        threadLocalReadyQueue->~ArrayQueue();
        // TODO: de-windows this 
        HeapFree(GetProcessHeap(), 0, threadLocalReadyQueue);

        Wado::Fiber::WdDeleteFiber(Wado::Fiber::WdGetCurrentFiber());        
    };

    unsigned long WorkerThreadStartFunction(void *param) {

        InitWorkerFiber();
        Wado::Thread::WdThreadLocalSetValue(TLcoreIndexID, param);
        ////std::cout << "Core ID for worker: " << (int) Wado::Thread::WdThreadLocalGetValue(TLcoreIndexID) << std::endl;
        Wado::Thread::WdThreadLocalSetValue(TLidleFiberID, Wado::Fiber::WdGetCurrentFiber());
        ////std::cout << "Idle fiber id for worker: " << Wado::Thread::WdThreadLocalGetValue(TLidleFiberID) << std::endl;
        IdleFiberFunction(param);

        return 0;
    };

    const std::vector<System::WdCoreInfo> InitFiberSystem(size_t maxWorkerCount) {
        assert(maxWorkerCount != 0);

        const std::vector<System::WdCoreInfo> availableCores = Wado::System::WdGetAvailableSystemCores();

        threadCount = (maxWorkerCount == ALL_AVAILABLE) ? availableCores.size() : maxWorkerCount;

        // TODO: should move the debug log init out of here 
        DebugLog::DebugLogInit(threadCount);

        // These are just x values rn, they will get deleted 
        DEBUG_GLOBAL(DebugLog::WD_MESSAGE, "FiberSystem", ("Decided thread count" + std::to_string(threadCount)).c_str());

        for (const System::WdCoreInfo& coreInfo : availableCores) {
            DEBUG_GLOBAL(DebugLog::WD_MESSAGE, "FiberSystem", ("For core " + std::to_string(coreInfo.coreNumber) + ", the hyperthread neighbour is: " + std::to_string(coreInfo.hyperthreadLocalNeighbour)).c_str());
        };

        // Allocate TLS indices for thread local stuff 
        TLlocalReadyQueueID = Wado::Thread::WdThreadLocalAllocate();
        TLpreviousFiberID = Wado::Thread::WdThreadLocalAllocate();
        TLallocatorID = Wado::Thread::WdThreadLocalAllocate();
        TLidleFiberID = Wado::Thread::WdThreadLocalAllocate();
        TLcoreIndexID = Wado::Thread::WdThreadLocalAllocate();
        FLqueueNodeID = Wado::Fiber::WdFiberLocalAllocate();

        // Allocate memory for containers

        // TODO: need to de-windows this

        Wado::Thread::WdThreadHandle currrentThreadHandle = Wado::Thread::WdGetCurrentThreadHandle();
        threadHandles[0] = currrentThreadHandle;

        // Set current thread affinity 

        Wado::Thread::WdSetThreadAffinity(currrentThreadHandle, availableCores[0].coreNumber);        
        // The current thread takes the first slot, now do all other threads 
        for (size_t i = 1; i < threadCount; ++i) {

            threadHandles[i] = Wado::Thread::WdCreateThread(WorkerThreadStartFunction, availableCores[i].coreNumber);
            
            if (threadHandles[i] == nullptr)  {
                throw std::runtime_error("Could not create worker threads");
            };

            Wado::Thread::WdSetThreadAffinity(threadHandles[i], availableCores[i].coreNumber);
        };

        InitWorkerFiber();
        // TODO: should make a pre-malloc for this case 
        // Pre/post malloc inits?
        WdReadyQueueItem readyQueueItem = static_cast<WdReadyQueueItem>(malloc(sizeof(Wado::Queue::Queue<void>::Item)));

        if (readyQueueItem == nullptr) {
            throw std::runtime_error("Could not create ready queue item for main task.");
        };

        readyQueueItem->data = Wado::Fiber::WdGetCurrentFiber();
        readyQueueItem->node = nullptr;

        Wado::Fiber::WdFiberLocalSetValue(FLqueueNodeID, readyQueueItem);
        Wado::Thread::WdThreadLocalSetValue(TLcoreIndexID, (void *) availableCores[0].coreNumber);

        Wado::Fiber::WdFiberID idleFiberID = Wado::Fiber::WdCreateFiber(IdleFiberFunction, nullptr);
        Wado::Thread::WdThreadLocalSetValue(TLidleFiberID, idleFiberID);

        // Start all other worker fibers 
        for (size_t i = 1; i < threadCount; ++i) {
            Wado::Thread::WdStartThread(threadHandles[i]);
        };

        return std::vector<System::WdCoreInfo>(availableCores.begin(), availableCores.begin() + threadCount);
    }; 

    void ShutdownFiberSystem() { 
        // Need to destroy all queue items etc here and send termination to all idle fibers 

        Wado::Atomics::TestAndSet(&close, 1);

        // TODO: un-windows this 

        size_t coreIndex = (size_t) Wado::Thread::WdThreadLocalGetValue(TLcoreIndexID);
        
        #ifndef NDEBUG
            std::cout << "Waiting on " << coreIndex << " for " << threadCount << " threads to finish" << std::endl;
        #endif;
        
        for (size_t i = 0; i < threadCount; ++i) {
            if (i != coreIndex) {
                #ifndef NDEBUG
                    std::cout << "Waiting for core " << i << std::endl;
                #endif;
                WaitForSingleObject(threadHandles[i], INFINITE);
            };
        };

        #ifndef NDEBUG
            std::cout << "Finished waiting and closing everything" << std::endl;
        #endif;

        for (size_t i = 0; i < threadCount; ++i) {
            if (i != coreIndex) {
                #ifndef NDEBUG
                    std::cout << "Closing thread on core " << i << std::endl;
                #endif;
                CloseHandle(threadHandles[i]);
            };
        };

        DebugLog::DebugLogShutdown();

        /*Wado::Fiber::WdFiberID currentIdleFiberID = Wado::Thread::WdThreadLocalGetValue(TLidleFiberID);
        
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
        Wado::Thread::WdThreadLocalFree(TLlocalReadyQueueID); */

        // Non-natural exit with this I think?
        //DeleteFiber(GetCurrentFiber()); 
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

        // //std::cout << "Signaled fence " << std::endl;

        if (fence == 0) {
            // //std::cout << "Reached count down" << std::endl;
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

    void WdFence::waitForSignal() noexcept {
        if (fence > 0) {
            while (Wado::Atomics::TestAndSet(&_waiterGuard, _signaled) == _signaled) { }; // busy spin
            ////std::cout << "About to wait" << std::endl;
            waiters.enqueue(static_cast<WdReadyQueueItem>(Wado::Fiber::WdFiberLocalGetValue(FLqueueNodeID))); 
            ////std::cout << "About to yield" << std::endl;
            ////std::cout << "About to yield for fence " << std::endl;
            // TODO: willYield here too 
            _waiterGuard = _unsignaled;
            FiberYield();
        };
    };
};