#include "Windows.h"

#include "System.h"
#include "Thread.h"
#include "Fiber.h"

#include "ArrayQueue.h"
#include "LinkedListQueue.h"
#include "LockFreeFIFO.h"

#include <stdexcept>

namespace Wado::FiberSystem {

    const size_t DEFAULT_LOCAL_READY_QUEUE_SIZE = 20;

    Wado::Thread::WdThreadLocalID TLlocalReadyQueueID;
    Wado::Thread::WdThreadLocalID TLpreviousFiberID;
    Wado::Thread::WdThreadLocalID TLallocatorID;
    Wado::Fiber::WdFiberLocalID FLqueueNodeID;

    Wado::Queue::LockFreeQueue<void> FiberGlobalReadyQueue;

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
        
        // Make own ready queue item now 
        // TODO: malloc should be initialized here and should be using WdMalloc 
        Wado::Queue::Queue<void>::Item *readyQueueItem = static_cast<Wado::Queue::Queue<void>::Item *>(malloc(sizeof(Wado::Queue::Queue<void>::Item)));

        if (readyQueueItem == nullptr) {
            throw std::runtime_error("Could not create item for new fiber");
        };

        readyQueueItem->data = currentFiberID;
        readyQueueItem->node = nullptr;

        Wado::Fiber::WdFiberLocalSetValue(FLqueueNodeID, readyQueueItem);
    };

    unsigned long WorkerThreadStartFunction(void *param) {

        InitWorkerFiber();

        Wado::Fiber::WdFiberID nextFiberID = PickNewFiber()->data;

        Wado::Thread::WdThreadLocalSetValue(TLpreviousFiberID, Wado::Fiber::WdGetCurrentFiber());

        //std::cout << "About to exit worker thread" << std::endl;
        Wado::Queue::Queue<void>::Item *readyQueueItem = static_cast<Wado::Queue::Queue<void>::Item *>(Wado::Fiber::WdFiberLocalGetValue(FLqueueNodeID));

        FiberGlobalReadyQueue.release(readyQueueItem);
        free(readyQueueItem);
        //std::cout << "Exiting initial worker thread" << std::endl;
        Wado::Fiber::WdSwitchFiber(nextFiberID);
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

        // Start all other worker fibers 
        for (int i = 1; i < threadCount; i++) {
            Wado::Thread::WdStartThread(threadHandles[i]);
        };
    }; 

    void ShutdownFiberSystem() { 

    };
};