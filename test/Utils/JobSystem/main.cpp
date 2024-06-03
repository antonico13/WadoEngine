#include <windows.h>

#include <iostream>
#include <bitset>

#include <stdexcept>

DWORD queueTlsIndex;
DWORD previousFiberTlsIndex;
DWORD coreNumberTlsIndex;
DWORD fiberItemFlsIndex;

CRITICAL_SECTION QueueSection; 

#include "ArrayQueue.h"
#include "LinkedListQueue.h"
#include "LockFreeFIFO.h"

typedef struct ThreadAffinityData {
    DWORD_PTR threadAffinity;
} THREADAFFINITYDATA, *PTHREADAFFINITYDATA;

typedef PTHREADAFFINITYDATA *PPTHREADAFFINITYDATA;

// global queue 
//Wado::Queue::LockFreeQueue<void> *globalQueue;

Wado::Queue::LockFreeQueue<void> *globalQueue;

static const size_t DEFAULT_LOCAL_FIBER_QUEUE = 20;

static volatile DWORD close = 0;

Wado::Queue::Queue<void>::Item* PickNewFiber() {
    Wado::Queue::Queue<void> *localReadyQueue = static_cast<Wado::Queue::Queue<void> *>(TlsGetValue(queueTlsIndex));
    
    if ((localReadyQueue == nullptr) && (GetLastError() != ERROR_SUCCESS)) {
        throw std::runtime_error("Could not get local ready queue");
    };
    
    if (!localReadyQueue->isEmpty()) {
        //std::cout << "Managed to choose fiber on local core " << reinterpret_cast<int>(TlsGetValue(coreNumberTlsIndex)) << std::endl;
        return localReadyQueue->dequeue();
    };
    Wado::Queue::Queue<void>::Item *nextFiber = nullptr;
    // Poll both queues
    bool waited = false;
    while ((nextFiber == nullptr) && (close == 0)) {
        while (globalQueue->isEmpty() && (close == 0)) {
            //std::cout << reinterpret_cast<int>(TlsGetValue(coreNumberTlsIndex)); 
            // if (!localReadyQueue->isEmpty()) {
            //     std::cout << "X" << std::endl;
            // };
        };
        // if (waited) {
        //     std::cout << "*" << reinterpret_cast<int>(TlsGetValue(coreNumberTlsIndex)) << std::endl;
        // };
        // if (!localReadyQueue->isEmpty()) {
        //     std::cout << "Busy wait with work to do" << std::endl;
        // };
        nextFiber = globalQueue->dequeue();
        waited = true;
    };
    //std::cout << "Stopped polling" << std::endl;
    if ( (close == 1) && (nextFiber == nullptr) ) {
        Wado::Queue::Queue<void>::Item *item = static_cast<Wado::Queue::Queue<void>::Item *>(FlsGetValue(fiberItemFlsIndex));
        globalQueue->release(item);
        free(item);

        
        Wado::Queue::ArrayQueue<void> * localQueuePtr = static_cast<Wado::Queue::ArrayQueue<void> *>(TlsGetValue(queueTlsIndex)); 
        localQueuePtr->~ArrayQueue();

        HeapFree(GetProcessHeap(), 0, localQueuePtr);
        
        DeleteFiber(GetCurrentFiber());
        ExitThread(0);
    };
    //std::cout << "Managed to choose fiber on core " << reinterpret_cast<int>(TlsGetValue(coreNumberTlsIndex)) << std::endl;
    // Need to do something the the node here, like free 
    //std::cout << "Picked from global queue" << std::endl;
    return nextFiber;
};

typedef struct FiberData {
    LPVOID fiberPtr;
} FiberData;

DWORD WINAPI ThreadAffinityTestFunction(LPVOID lpParam) {
    PTHREADAFFINITYDATA affinityData = (PTHREADAFFINITYDATA) lpParam;
    DWORD_PTR threadAffinity = affinityData->threadAffinity >> 1;
    DWORD64 coreNumber = 0;

    while (threadAffinity > 0) {
        coreNumber++;
        threadAffinity = threadAffinity >> 1;
    };

    //std::cout << "Running thread on core:" << coreNumber << std::endl;

    if (!TlsSetValue(coreNumberTlsIndex, (LPVOID) coreNumber)) {
        throw std::runtime_error("Could not set tls core numebr value");
    };

    LPVOID lpvQueue; 
    lpvQueue = (LPVOID) HeapAlloc(GetProcessHeap(), 0, sizeof(Wado::Queue::ArrayQueue<void>));

    if (lpvQueue == NULL) {
        throw std::runtime_error("Could not allocate memory for local queues");
    };

    new (lpvQueue) Wado::Queue::ArrayQueue<void>(DEFAULT_LOCAL_FIBER_QUEUE);

    if (!TlsSetValue(queueTlsIndex, lpvQueue)) {
        throw std::runtime_error("Could not set tls value");
    };

    LPVOID fiberID = ConvertThreadToFiber(NULL);

    Wado::Queue::Queue<void>::Item  *item = static_cast<Wado::Queue::Queue<void>::Item *>(malloc(sizeof(Wado::Queue::Queue<void>::Item)));

    if (item == nullptr) {
        throw std::runtime_error("Could not create item for worker thread fiber");
    };

    item->data = fiberID;
    item->node = nullptr;

    if (!FlsSetValue(fiberItemFlsIndex, item)) {
        throw std::runtime_error("Could not set fiber item for main thread");
    };
    
    LPVOID nextFiber = PickNewFiber()->data;
    if (!TlsSetValue(previousFiberTlsIndex, GetCurrentFiber())) {
        throw std::runtime_error("Could not set previous task pointer when switching tasks");
    };

    std::cout << "About to exit worker thread" << std::endl;
    globalQueue->release(item);
    free(item);
    std::cout << "Exiting initial worker thread" << std::endl;
    SwitchToFiber(nextFiber);

    return 0;
};

void FiberYield() {
    LPVOID nextFiber = PickNewFiber()->data;
    if (!TlsSetValue(previousFiberTlsIndex, 0)) {
            throw std::runtime_error("Could not set previous task pointer when switching tasks");
    };
    if (GetCurrentFiber() != nextFiber) {
        SwitchToFiber(nextFiber); 
    };
};

class HybridLock {
    public:
        HybridLock() noexcept {
            _lock = _unlocked;
            _waitGuard = _unlocked;
            _willYield = _unlocked;
            _ownerFiber = nullptr;
        };

        ~HybridLock() noexcept {
            // TODO: good?
            _waiters.~LinkedListQueue<void>();
        };

        static const size_t DEFAULT_TIME_LIMIT = 500;

        void tryAcquire(size_t timeLimit = DEFAULT_TIME_LIMIT) {
            // while ((timeLimit--) && (InterlockedCompareExchange(&_lock, _locked, _unlocked) == _locked)) {  // Doing a lot of CAS here 
            //     // just spin 
            // };
            // if (timeLimit == 0) { // TODO what happens when both go false at the same time?
            //     InterlockedExchange(&_haveWaiters, 1); // TODO: look into using different types for performance 
            //     _waiters.enqueue(static_cast<Wado::Queue::Queue<void>::Item *>(FlsGetValue(fiberItemFlsIndex))); // Could have contention here. multiple cores can try to get the 
            //     // waiter queue. 
            //     FiberYield();
            // };
            // _ownerFiber = GetCurrentFiber();
        };

        void acquire() {
            while (InterlockedExchange(&_waitGuard, _locked) == _locked) {
                // spin  
            };

            if (_lock == _locked) {
                _waiters.enqueue(static_cast<Wado::Queue::Queue<void>::Item *>(FlsGetValue(fiberItemFlsIndex)));    
                // Need to say will yield here 
                InterlockedExchange(&_waitGuard, _unlocked);      
                //if (_willYield == _locked) {
                    FiberYield();
                //};
            } else {
                InterlockedExchange(&_lock, _locked);
                InterlockedExchange(&_waitGuard, _unlocked);      
            };
            _ownerFiber = GetCurrentFiber();
        };

        void release() {
            if (_ownerFiber != GetCurrentFiber()) {
                throw std::runtime_error("Tried to release a lock the fiber didn't onw");
            };
            while (InterlockedExchange(&_waitGuard, _locked) == _locked) {
                // spin  
            };

            if (_waiters.isEmpty()) {
                InterlockedExchange(&_lock, _unlocked);
            } else {
                Wado::Queue::Queue<void> *localReadyQueue = static_cast<Wado::Queue::Queue<void> *>(TlsGetValue(queueTlsIndex));
                _ownerFiber = nullptr;
                Wado::Queue::Queue<void>::Item *nextFiber = _waiters.dequeue();
                // if (_willYield == _unlocked) {
                // if (localReadyQueue->isFull()) {
                    globalQueue->enqueue(nextFiber);
                // } else {
                //     localReadyQueue->enqueue(nextFiber); 
                // };
            };
            InterlockedExchange(&_waitGuard, _unlocked);
        };
    private:
        // TODO: these dont have to be longs, can do 16 too 
        const LONG _unlocked = 0;
        const LONG _locked = 1;

        LONG volatile _waitGuard;
        LONG volatile _willYield;
        LONG volatile _lock;
        LPVOID _ownerFiber;
        // This is kinda wild 
        Wado::Queue::LinkedListQueue<void> _waiters;
};

static HybridLock globalLock = HybridLock();
static int count = 0;

static int fiberCount[8];
static int unlockCount[8];

class Fence {
    public:
        Fence(size_t count) {
            fence = count;
            _waiterGuard = _unsignaled;
        };

        ~Fence() noexcept {

        };

        void signal() noexcept {
            InterlockedDecrement(&fence);

            while (InterlockedExchange(&_waiterGuard, _signaled) == _signaled) { 
                //busy spin 
            };

            std::cout << "Signaled fence " << std::endl;

            if (fence == 0) {
                std::cout << "Reached count down" << std::endl;
                Wado::Queue::Queue<void> *localReadyQueue = static_cast<Wado::Queue::Queue<void> *>(TlsGetValue(queueTlsIndex));
                while (!waiters.isEmpty()) {
                    Wado::Queue::Queue<void>::Item *toWakeFiber = waiters.dequeue();
                    //if (localReadyQueue->isFull()) {
                        globalQueue->enqueue(toWakeFiber);
                    // } else {
                    //     localReadyQueue->enqueue(toWakeFiber); 
                    // };
                }; 
            };
            _waiterGuard = _unsignaled;
        };

        void waitForSignal() {
            if (fence > 0) {
                while (InterlockedExchange(&_waiterGuard, _signaled) == _signaled) { 
                    //busy spin 
                };
                waiters.enqueue(static_cast<Wado::Queue::Queue<void>::Item *>(FlsGetValue(fiberItemFlsIndex))); 
                std::cout << "About to yield for fence " << std::endl;
                // TODO: willYield here too 
                _waiterGuard = _unsignaled;
                FiberYield();
            };
        };
    private:

        const LONG _unsignaled = 0;
        const LONG _signaled = 1;
        
        LONG volatile fence;
        LONG volatile _waiterGuard;
        // This is kinda wild 
        Wado::Queue::LinkedListQueue<void> waiters;
};

// two pointer derefs with this, 
// will hit really bad performance 
typedef struct fiberArgs {
    LPVOID mainArgs; 
    LPVOID fenceToSignal; 
    LPVOID item;
} fiberArgs;

#define WIN32_TASK(TaskName, ArgumentType, ArgumentName, Code)\
    void TaskName(LPVOID fiberParam) {\
        LPVOID previousFiberPtr = TlsGetValue(previousFiberTlsIndex);\
        if ((previousFiberPtr != 0) && (GetLastError() == ERROR_SUCCESS)) {\
            DeleteFiber(previousFiberPtr);\
        };\
        fiberArgs *args = static_cast<fiberArgs *>(fiberParam);\
        if (!FlsSetValue(fiberItemFlsIndex, args->item)) {\
            throw std::runtime_error("Could not set fiber local data");\
        };\
        ArgumentType *ArgumentName = static_cast<ArgumentType *>(args->mainArgs);\
        Code;\
        if (args->fenceToSignal != 0) {\
            Fence *fence = static_cast<Fence *>(args->fenceToSignal);\
            fence->signal();\
        };\
        LPVOID nextFiber = PickNewFiber()->data;\
        if (!TlsSetValue(previousFiberTlsIndex, GetCurrentFiber())) {\
            throw std::runtime_error("Could not set previous task pointer when switching tasks");\
        };\
        Wado::Queue::Queue<void>::Item *item = static_cast<Wado::Queue::Queue<void>::Item *>(FlsGetValue(fiberItemFlsIndex));\
        globalQueue->release(item);\
        free(item);\
        free(args);\
        if (GetCurrentFiber() != nextFiber) {\
            SwitchToFiber(nextFiber);\
        };\
    };


#define TASK(TaskName, ArgumentType, ArgumentName, Code) WIN32_TASK(TaskName, ArgumentType, ArgumentName, Code)

typedef struct DummyData {
    int x; 
    int y;
} DummyData;

TASK(SillyTask, DummyData, data, {
    std::cout << "Silly!" << std::endl;
});

// Arguments and fence memory is managed by the caller of this
void makeTask(LPFIBER_START_ROUTINE function, void *arguments, Fence* fenceToSignal = nullptr) {
    fiberArgs* args = static_cast<fiberArgs *>(malloc(sizeof(fiberArgs)));
    if (args == nullptr) {
        throw std::runtime_error("Could not allocate space for function arguments");
    };

    Wado::Queue::Queue<void>::Item *item = static_cast<Wado::Queue::Queue<void>::Item *>(malloc(sizeof(Wado::Queue::Queue<void>::Item)));

    if (item == nullptr) {
        throw std::runtime_error("Could not create item for new fiber");
    };

    args->mainArgs = arguments;
    args->fenceToSignal = fenceToSignal;
    args->item = item;

    // 64 kb stack 
    LPVOID fiberPtr = CreateFiber(0, function, args);

    item->data = fiberPtr;
    item->node = nullptr;

    // push to global queue here if you cant to normal one

    Wado::Queue::Queue<void> *localReadyQueue = static_cast<Wado::Queue::Queue<void> *>(TlsGetValue(queueTlsIndex));
    if (localReadyQueue->isFull()) {
        globalQueue->enqueue(item);
    } else {
        localReadyQueue->enqueue(item); 
    };
};

TASK(DummyTask, DummyData, data, {
    int coreNumber = reinterpret_cast<int>(TlsGetValue(coreNumberTlsIndex));
    fiberCount[coreNumber]++;
    globalLock.acquire();
    count++;
    std::cout << "Started on: " << coreNumber << ", so far this many fibers also started here: " << fiberCount[coreNumber] << std::endl;
    std::cout << "TLS value: " << TlsGetValue(coreNumberTlsIndex) << std::endl;
    std::cout << "Fiber number: " << count << " overall " << std::endl;
    unlockCount[reinterpret_cast<int>(TlsGetValue(coreNumberTlsIndex))]++;
    std::cout << "Got unlocked on: " << reinterpret_cast<int>(TlsGetValue(coreNumberTlsIndex)) << ", so far this many fibers also unlocked here: " << unlockCount[reinterpret_cast<int>(TlsGetValue(coreNumberTlsIndex))] << std::endl;
    globalLock.release();
    //makeTask(SillyTask, data);
});

int main() {
    std::cout << "Started: " << std::endl;
    HANDLE currentProcess = GetCurrentProcess();
    std::cout << "Current process pseudo handle: " << currentProcess << std::endl;
    DWORD_PTR processAffinityMask;
    DWORD_PTR systemAffinityMask;
    BOOL res;
    res = GetProcessAffinityMask(currentProcess, &processAffinityMask, &systemAffinityMask);

    std::bitset<sizeof(DWORD_PTR)> processSet(processAffinityMask);
    std::bitset<sizeof(DWORD_PTR)> systemSet(systemAffinityMask);

    std::cout << "Affinity mask get results: " << res << std::endl;
    std::cout << "Process affinity mask: " << processSet << std::endl;
    std::cout << "System affinity mask: " << systemSet << std::endl;

    // allocate one tls index for the thread ready queue 
     
    if ((queueTlsIndex = TlsAlloc()) == TLS_OUT_OF_INDEXES) {
        throw std::runtime_error("Out of indices for queue thread local storage index");
    };

    if ((previousFiberTlsIndex = TlsAlloc()) == TLS_OUT_OF_INDEXES) {
        throw std::runtime_error("Out of indices for fiber pointer thread local storage index");
    };  

    if ((coreNumberTlsIndex = TlsAlloc()) == TLS_OUT_OF_INDEXES) {
        throw std::runtime_error("Out of indices core number thread local storage index");
    };  

    if ((fiberItemFlsIndex = FlsAlloc(NULL)) == FLS_OUT_OF_INDEXES) {
        throw std::runtime_error("Ran out of indices for FLS item storage");
    };

    DWORD threadCount = 0;

    PPTHREADAFFINITYDATA threadAffinityData;
    PDWORD  threadIDArray;
    PHANDLE  threadHandleArray; 

    for (size_t i = 0; i < processSet.size(); i++) {
        threadCount += processSet[i];
    };

    std::cout << "Total core count: " << threadCount << std::endl;

    globalQueue = static_cast<Wado::Queue::LockFreeQueue<void> *>(HeapAlloc(GetProcessHeap(), 0, sizeof(Wado::Queue::LockFreeQueue<void>)));

    if (globalQueue == nullptr) {
        throw std::runtime_error("Could not allocate global scheduler queue");
    };

    new (globalQueue) Wado::Queue::LockFreeQueue<void>(threadCount);

    size_t availableCores[processSet.size()];
    size_t coreIndex = 0;
    
    for (size_t i = 0; i < processSet.size(); i++) {
        if (processSet[i]) {    
            availableCores[coreIndex] = i;
            ++coreIndex;
        };
    };

    // Allocate memory for containers

    threadAffinityData = (PPTHREADAFFINITYDATA) HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, threadCount * sizeof(PTHREADAFFINITYDATA));

    if (threadAffinityData == NULL) {
        throw std::runtime_error("Could not allocate memory to hold thread affinity data container");
    };

    threadIDArray = (PDWORD) HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, threadCount * sizeof(DWORD));

    if (threadIDArray == NULL) {
        throw std::runtime_error("Could not allocate memory to hold thread ID array");
    };

    threadIDArray[0] = GetCurrentThreadId();

    threadHandleArray = (PHANDLE) HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, threadCount * sizeof(HANDLE));

    if (threadHandleArray == NULL) {
        throw std::runtime_error("Could not allocate memory to hold thread handle array");
    };

    //threadHandleArray[0] GetCurrentThread();
    
    if (!DuplicateHandle(GetCurrentProcess(), GetCurrentThread(), GetCurrentProcess(), threadHandleArray, NULL, FALSE, DUPLICATE_SAME_ACCESS)) {
        throw std::runtime_error("Could not duplicate main thread handle");
    };

    threadAffinityData[0] = (PTHREADAFFINITYDATA) HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(THREADAFFINITYDATA));

    if(threadAffinityData[0] == NULL) {
        throw std::runtime_error("Could not allocate thread affinity data struct for the first thread");
    };

    threadAffinityData[0]->threadAffinity = (DWORD)1 << availableCores[0];

    DWORD_PTR oldThreadAffinity = SetThreadAffinityMask(threadHandleArray[0], threadAffinityData[0]->threadAffinity);
        
    std::cout << "Old thread affinity for thread " << 0 << " was " << std::bitset<sizeof(DWORD_PTR)>(oldThreadAffinity) << std::endl;

    if (oldThreadAffinity == 0) {
        throw std::runtime_error("Could not set thread affinity for the first thread");
    };

    // The current thread takes the first slot 
    for (int i = 1; i < threadCount; i++) {
        // Allocate memory for all thread related data 

        threadAffinityData[i] = (PTHREADAFFINITYDATA) HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(THREADAFFINITYDATA));

        if(threadAffinityData[i] == NULL) {
            throw std::runtime_error("Could not allocate thread affinity data struct");
        };

        threadAffinityData[i]->threadAffinity = (DWORD)1 << availableCores[i];

        // Create threads in suspended mode 

        threadHandleArray[i] = CreateThread(NULL, 0, ThreadAffinityTestFunction, threadAffinityData[i], CREATE_SUSPENDED, &threadIDArray[i]);
        
        if (threadHandleArray[i] == NULL)  {
           throw std::runtime_error("Could not create threads");
        };

        DWORD_PTR oldThreadAffinity = SetThreadAffinityMask(threadHandleArray[i], threadAffinityData[i]->threadAffinity);
        
        std::cout << "Old thread affinity for thread " << i << " was " << std::bitset<sizeof(DWORD_PTR)>(oldThreadAffinity) << std::endl;

        if (oldThreadAffinity == 0) {
            throw std::runtime_error("Could not set thread affinity");
        };
    };

    // allocate own now 
    DWORD_PTR threadAffinity = threadAffinityData[0]->threadAffinity >> 1;
    DWORD64 coreNumber = 0;

    while (threadAffinity > 0) {
        coreNumber++;
        threadAffinity = threadAffinity >> 1;
    };

    if (!TlsSetValue(coreNumberTlsIndex, (LPVOID) coreNumber)) {
        throw std::runtime_error("Could not set tls core numebr value for thread 0");
    };

    LPVOID lpvQueue; 
    lpvQueue = (LPVOID) HeapAlloc(GetProcessHeap(), 0, sizeof(Wado::Queue::ArrayQueue<void>));

    if (lpvQueue == NULL) {
        throw std::runtime_error("Could not allocate memory for local queues for main thread");
    };

    new (lpvQueue) Wado::Queue::ArrayQueue<void>(DEFAULT_LOCAL_FIBER_QUEUE);

    if (!TlsSetValue(queueTlsIndex, lpvQueue)) {
        throw std::runtime_error("Could not set tls value for local queue on main thread");
    };

    std::cout << "Running main thread on core: " << coreNumber << std::endl;

    std::cout << "Making 500 main fences" << std::endl;

    Fence *fence = static_cast<Fence *>(HeapAlloc(GetProcessHeap(), 0, sizeof(Fence)));

    if (fence == nullptr) {
        std::cout << "Could not allocate fence" << std::endl;
    };

    if (!InitializeCriticalSectionAndSpinCount(&QueueSection, 0x00000400) ) {
        throw std::runtime_error("Could not create critical section");
    };

    for (int i = 1; i < threadCount; i++) {
        ResumeThread(threadHandleArray[i]);
    };

    // Now, convert this thread to a fiber 

    LPVOID fiberID = ConvertThreadToFiber(NULL);

    Wado::Queue::Queue<void>::Item *item = static_cast<Wado::Queue::Queue<void>::Item *>(malloc(sizeof(Wado::Queue::Queue<void>::Item)));

    if (item == nullptr) {
        throw std::runtime_error("Could not create item for new fiber");
    };

    item->data = fiberID;
    item->node = nullptr;

    if (!FlsSetValue(fiberItemFlsIndex, item)) {
        throw std::runtime_error("Could not set fiber item for main thread");
    };
    
    // Now, we are a fiber, do main fiber loop 
    std::cout << "Main thread became fiber " << fiberID << std::endl;

    std::cout << "Main thread item: " << item->data << std::endl;

    DummyData data;

    const size_t taskCount = 800;

    new (fence) Fence(taskCount);

    for (size_t i = 0; i < 800; i++) {
        makeTask(DummyTask, &data, fence);
    };

    fence->waitForSignal();

    fence->~Fence();

    HeapFree(GetProcessHeap(), 0, fence);
    
    //WaitForMultipleObjects(threadCount, threadHandleArray, TRUE, INFINITE);

    // PSYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX systemLogicalInformationBuffer = (PSYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX) HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, 100 * sizeof(SYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX));

    // if (systemLogicalInformationBuffer == NULL) {
    //     throw std::runtime_error("Could not allocate memory to hold the system logical processor info");
    // };

    // DWORD bufferSize = 100 * sizeof(SYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX);

    // std::cout << "Buffersize: " << bufferSize << std::endl;

    // if (!GetLogicalProcessorInformationEx(RelationAll, systemLogicalInformationBuffer, &bufferSize)) {
    //     throw std::runtime_error("Could not get processor info");
    // };

    // std::cout << "Buffersize: " << bufferSize << std::endl;

    // char* bufferPtr = (char*) systemLogicalInformationBuffer;
    
    // size_t i = 0;
    // while (i < bufferSize) {
    //     PSYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX processInfo = ((PSYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX)(bufferPtr + i));
    //     std::cout << "Relationship type: " << processInfo->Relationship << std::endl;
    //     switch (processInfo->Relationship) {
    //         case RelationProcessorCore: case 7:
    //             // TODO: figure out how to get the modules together 
    //             PROCESSOR_RELATIONSHIP processorRelationship = processInfo->Processor;
    //             if (processorRelationship.Flags == LTP_PC_SMT) {
    //                 std::cout << "PC Supports SMT" << std::endl;
    //             } else {
    //                 std:: cout << "No SMT support" << std::endl;
    //             };

    //             std::cout << "Efficiency class " << (DWORD) processorRelationship.EfficiencyClass << std::endl;
    //             std::cout << "Group count " << processorRelationship.GroupCount << std::endl;
    //             break;
    //         case RelationNumaNode:
    //             NUMA_NODE_RELATIONSHIP numaNode = processInfo->NumaNode;
    //             std::cout << "NUMA node number: " << numaNode.NodeNumber << std::endl;
    //             GROUP_AFFINITY affinity = numaNode.GroupMask;
    //             std::cout << "Affinity: " << std::bitset<8>(affinity.Mask) << std::endl;
    //             std::cout << "Group: " << affinity.Group << std::endl;
    //             break;
    //         case RelationCache:
    //             CACHE_RELATIONSHIP cacheInfo = processInfo->Cache;
    //             std::cout << "Cache level: " << (DWORD)cacheInfo.Level << std::endl;
    //             std::cout << "Cache size:" << cacheInfo.CacheSize << std::endl;
    //             std::cout << "Line size: " << cacheInfo.LineSize << std::endl;
    //             std::cout << "Associativity: " << (DWORD)cacheInfo.Associativity << std::endl;
    //             //PROCESSOR_CACHE_TYPE cacheType = cacheInfo.Type;

    //             switch (cacheInfo.Type) {
    //                 case CacheUnified:
    //                     std::cout << "Unified cache" << std::endl;
    //                     break;
    //                 case CacheInstruction:
    //                     std::cout << "Instruction cache" << std::endl;
    //                     break;
    //                 case CacheData:
    //                     std::cout << "Data cache" << std::endl;
    //                     break;
    //                 case CacheTrace:
    //                     std::cout << "Trace cache" << std::endl;
    //                     break;
    //                 // default:
    //                 //     std::cout << "Unknown" << std::endl;
    //                 //     break;
    //             };
    //             break;
    //         case RelationGroup:
    //             GROUP_RELATIONSHIP groupRelation = processInfo->Group;
    //             std::cout << "Maximum group count: " << groupRelation.MaximumGroupCount << std::endl;
    //             std::cout << "Active group count: " << groupRelation.ActiveGroupCount << std::endl;
    //             break;
    //         default: 
    //             std::cout << "Not yet" << std::endl;
    //     };
    //     i += processInfo->Size;
    //     //std:: cout << "Current i size " << i << std::endl;
    // };

    InterlockedExchange(&close, 1);

    int core = (int) TlsGetValue(coreNumberTlsIndex);
    std::cout << "Waiting on " << core << " for threads to finish" << std::endl;
    for (int i = 0; i < threadCount; i++) {
        if (i != core) {
            WaitForSingleObject(threadHandleArray[i], INFINITE);
        };
    };

    std::cout << "Finished waiting and closing everything" << std::endl;

    for (int i = 0; i < threadCount; i++) {
        if (i != core) {
            CloseHandle(threadHandleArray[i]);
        };
    };

    Wado::Queue::Queue<void>::Item *localItem = static_cast<Wado::Queue::Queue<void>::Item *>(FlsGetValue(fiberItemFlsIndex));
    globalQueue->release(localItem);
    free(localItem);

    Wado::Queue::ArrayQueue<void> * localQueuePtr = static_cast<Wado::Queue::ArrayQueue<void> *>(TlsGetValue(queueTlsIndex)); 

    std::cout << "Local queue is empty: " << localQueuePtr->isEmpty() << std::endl;
    // Have to call destructor manually 
    localQueuePtr->~ArrayQueue();

    std::cout << "Global queue should be empty: " << globalQueue->isEmpty() << std::endl;

    globalQueue->~LockFreeQueue<void>();
    
    HeapFree(GetProcessHeap(), 0, TlsGetValue(queueTlsIndex));

    HeapFree(GetProcessHeap(), 0, threadIDArray);
    HeapFree(GetProcessHeap(), 0, threadHandleArray);

    HeapFree(GetProcessHeap(), 0, globalQueue);

    //HeapFree(GetProcessHeap(), 0, systemLogicalInformationBuffer);

    FlsFree(fiberItemFlsIndex);

    TlsFree(queueTlsIndex);
    TlsFree(previousFiberTlsIndex);
    TlsFree(coreNumberTlsIndex);


    std::cout << "Waited for all objects" << std::endl;

    for (int i = 0; i < 8; i++) {
        std::cout << "Core: " << i << " started " << fiberCount[i] << " fibers " << std::endl;
        std::cout << "Core: " << i << " unlocked " << unlockCount[i] << " fibers " << std::endl;
    };

    DeleteCriticalSection(&QueueSection);

    for (int i = 0; i < threadCount; i++) {
        if (threadAffinityData[i] != NULL) {
            HeapFree(GetProcessHeap(), 0, threadAffinityData[i]);
            threadAffinityData[i] = NULL; // Ensure address is not reused.
        };
    };

    HeapFree(GetProcessHeap(), 0, threadAffinityData);

    std::cout << "Total fibers: " << count << std::endl;

    DeleteFiber(GetCurrentFiber());
    ExitThread(0);

    return 0;
};