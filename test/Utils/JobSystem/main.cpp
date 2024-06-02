#include <windows.h>

#include <iostream>
#include <bitset>

#include <stdexcept>

DWORD queueTlsIndex;
DWORD previousFiberTlsIndex;
DWORD coreNumberTlsIndex;

#include "ArrayQueue.h"
#include "LinkedListQueue.h"
#include "LockFreeFIFO.h"

typedef struct ThreadAffinityData {
    DWORD_PTR threadAffinity;
} THREADAFFINITYDATA, *PTHREADAFFINITYDATA;

typedef PTHREADAFFINITYDATA *PPTHREADAFFINITYDATA;

// global queue 
Wado::Queue::LockFreeQueue<void> *globalQueue;

static const size_t DEFAULT_LOCAL_FIBER_QUEUE = 20;

LPVOID PickNewFiber() {
    Wado::Queue::Queue<void> *localReadyQueue = static_cast<Wado::Queue::Queue<void> *>(TlsGetValue(queueTlsIndex));
    
    if ((localReadyQueue == nullptr) && (GetLastError() != ERROR_SUCCESS)) {
        throw std::runtime_error("Could not get local ready queue");
    };
    
    if (!localReadyQueue->isEmpty()) {
        return static_cast<LPVOID>(localReadyQueue->dequeue());
    };
    Wado::Queue::LockFreeQueue<void>::Item nextFiber;
    nextFiber.data = nullptr;
    nextFiber.node = nullptr;
    while ( (nextFiber = globalQueue->dequeue()).data == nullptr ) { 
        // busy wait
    };
    // Need to do something the the node here, like free 
    return nextFiber.data;
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

    std::cout << "Running thread on core:" << coreNumber << std::endl;

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

    // Now, we are a fiber, do main fiber loop 
    std::cout << "Became fiber" << std::endl;

    LPVOID fiberPtr = PickNewFiber();

    SwitchToFiber(fiberPtr);

    LPVOID localQueuePtr = TlsGetValue(queueTlsIndex); 

    // Have to call destructor manually 
    static_cast<Wado::Queue::ArrayQueue<void> *>(localQueuePtr)->~ArrayQueue();

    HeapFree(GetProcessHeap(), 0, TlsGetValue(queueTlsIndex));

    return 0;
};

void FiberYield() {
    LPVOID nextFiber = PickNewFiber();
    if (!TlsSetValue(previousFiberTlsIndex, 0)) {
            throw std::runtime_error("Could not set previous task pointer when switching tasks");
    };
    SwitchToFiber(nextFiber);
};

class HybridLock {
    public:
        HybridLock() noexcept {
            lock = _unlocked;
            ownerFiber = nullptr;
        };

        ~HybridLock() noexcept {
            // TODO: good?
            waiters.~LinkedListQueue();
        };

        static const size_t DEFAULT_TIME_LIMIT = 500;

        void tryAcquire(size_t timeLimit = DEFAULT_TIME_LIMIT) {
            while ((timeLimit--) && InterlockedCompareExchange(&lock, _locked, _unlocked)) { };
            if (timeLimit == 0) { // TODO what happens when both go false at the same time?
                waiters.enqueue(GetCurrentFiber());
                FiberYield();
            };
            ownerFiber = GetCurrentFiber();
        };

        void acquire() {
            if (InterlockedCompareExchange(&lock, _locked, _unlocked)) {
                waiters.enqueue(GetCurrentFiber());
                FiberYield();
            };
            ownerFiber = GetCurrentFiber();
        };

        void release() {
            if (ownerFiber != GetCurrentFiber()) {
                throw std::runtime_error("Tried to release a lock the fiber didn't onw");
            };
            if (!waiters.isEmpty()) { // Maybe this should be atomic? I can have race conditions here 
                // need to add a fiber to some ready list here
                Wado::Queue::Queue<void> *localReadyQueue = static_cast<Wado::Queue::Queue<void> *>(TlsGetValue(queueTlsIndex));
                if (localReadyQueue->isFull()) {
                    globalQueue->enqueue({nullptr, waiters.dequeue()});
                } else {
                    localReadyQueue->enqueue(waiters.dequeue()); 
                };
            } else {
                InterlockedCompareExchange(&lock, _unlocked, _locked);
            };
        };
    private:
        // TODO: these dont have to be longs, can do 16 too 
        const LONG _unlocked = 0;
        const LONG _locked = 1;

        LONG volatile lock;
        LPVOID ownerFiber;
        // This is kinda wild 
        Wado::Queue::LinkedListQueue<void> waiters;
};

static HybridLock globalLock = HybridLock();
static int count = 0;

static int fiberCount[8];

class Fence {
    public:
        Fence() {
            fence = _unsignaled;
        };

        ~Fence() noexcept {

        };

        void reset() noexcept {
            InterlockedExchange(&fence, _unsignaled);
        };

        void signal() noexcept {
            InterlockedExchange(&fence, _signaled);
            //std::cout << "Called signal fence" << std::endl;
            Wado::Queue::Queue<void> *localReadyQueue = static_cast<Wado::Queue::Queue<void> *>(TlsGetValue(queueTlsIndex));
            while (!waiters.isEmpty()) {
                //std::cout << "At least one waiter" << std::endl;
                if (localReadyQueue->isFull()) {
                    //std::cout << "Local queue is full" << std::endl;
                    globalQueue->enqueue({nullptr, waiters.dequeue()});
                } else {
                    //std::cout << "Local queue is not full" << std::endl;
                    localReadyQueue->enqueue(waiters.dequeue()); 
                };
            };
        }

        void waitForSignal() {
            if (!InterlockedAnd(&fence, _signaled)) {
                waiters.enqueue(GetCurrentFiber());
                FiberYield();
            };
        };
    private:

        const LONG _unsignaled = 0;
        const LONG _signaled = 1;
        
        LONG volatile fence;
        // This is kinda wild 
        Wado::Queue::LinkedListQueue<void> waiters;
};

// two pointer derefs with this, 
// will hit really bad performance 
typedef struct fiberArgs {
    LPVOID mainArgs; 
    LPVOID fenceToSignal; 
} fiberArgs;

#define WIN32_TASK(TaskName, ArgumentType, ArgumentName, Code)\
    void TaskName(LPVOID fiberParam) {\
        LPVOID previousFiberPtr = TlsGetValue(previousFiberTlsIndex);\
        if ((previousFiberPtr != 0) && (GetLastError() == ERROR_SUCCESS)) {\
            DeleteFiber(previousFiberPtr);\
        };\
        fiberArgs *args = static_cast<fiberArgs *>(fiberParam);\
        ArgumentType *ArgumentName = static_cast<ArgumentType *>(args->mainArgs);\
        Code;\
        if (args->fenceToSignal != 0) {\
            Fence *fence = static_cast<Fence *>(args->fenceToSignal);\
            fence->signal();\
        };\
        free(args);\
        LPVOID nextFiber = PickNewFiber();\
        if (!TlsSetValue(previousFiberTlsIndex, GetCurrentFiber())) {\
            throw std::runtime_error("Could not set previous task pointer when switching tasks");\
        };\
        SwitchToFiber(nextFiber);\
    };

#define TASK(TaskName, ArgumentType, ArgumentName, Code) WIN32_TASK(TaskName, ArgumentType, ArgumentName, Code)

typedef struct DummyData {
    int x; 
    int y;
} DummyData;

TASK(SillyTask, DummyData, data, {
    //globalLock.acquire();
    //count++;
    int coreNumber = reinterpret_cast<int>(TlsGetValue(coreNumberTlsIndex));
    fiberCount[coreNumber % 8]++;
    std::cout << "Silly!" << std::endl;
    //globalLock.release();
});

// Arguments and fence memory is managed by the caller of this
void makeTask(LPFIBER_START_ROUTINE function, void *arguments, Fence* fenceToSignal = nullptr) {
    fiberArgs* args = static_cast<fiberArgs *>(malloc(sizeof(fiberArgs)));
    if (args == nullptr) {
        throw std::runtime_error("Could not allocate space for function arguments");
    };

    args->mainArgs = arguments;
    args->fenceToSignal = fenceToSignal;

    // 64 kb stack 
    LPVOID fiberPtr = CreateFiber(65536, function, args);

    //std::cout << "Made new task: " << fiberPtr << std::endl;

    // push to global queue here if you cant to normal one

    Wado::Queue::Queue<void> *localReadyQueue = static_cast<Wado::Queue::Queue<void> *>(TlsGetValue(queueTlsIndex));
    if (localReadyQueue->isFull()) {
        //std::cout << "local queue is full" << std::endl;
        globalQueue->enqueue({nullptr, fiberPtr});
    } else {
        //std::cout << "Local queue is not full" << std::endl;
        localReadyQueue->enqueue(fiberPtr); 
    };
};

TASK(DummyTask, DummyData, data, {
    //globalLock.acquire();
    //count++;
    int coreNumber = reinterpret_cast<int>(TlsGetValue(coreNumberTlsIndex));
    fiberCount[coreNumber % 8]++;
    std::cout << "Fiber number: " << fiberCount[coreNumber % 8] << " :) " << std::endl;
    std::cout << "Running on core: " << coreNumber << std::endl;
    makeTask(SillyTask, data);
    //globalLock.release();
});

int main() {
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

    threadHandleArray[0] = GetCurrentThread();

    
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

    std::cout << "Running main thread on core: " << threadAffinityData[0]->threadAffinity << std::endl;

    std::cout << "Making 500 main fences" << std::endl;

    Fence *fenceData = static_cast<Fence *>(HeapAlloc(GetProcessHeap(), 0, 400 * sizeof(Fence)));

    if (fenceData == nullptr) {
        std::cout << "Could not allocate fences" << std::endl;
    };

    for (int i = 1; i < threadCount; i++) {
        ResumeThread(threadHandleArray[i]);
    };

    // Now, convert this thread to a fiber 

    LPVOID fiberID = ConvertThreadToFiber(NULL);

    // Now, we are a fiber, do main fiber loop 
    std::cout << "Main thread became fiber" << std::endl;

    DummyData data;

    for (size_t i = 0; i < 400; i++) {
        new (fenceData + i) Fence();
        ///std::cout << "Fence address: " << fenceData + i << std::endl;
        //(fenceData + i)->signal();
        makeTask(DummyTask, &data, fenceData + i);
    };

    for (size_t i = 0; i < 400; i++) {
        (fenceData + i)->waitForSignal();
    };
    
    //WaitForMultipleObjects(threadCount, threadHandleArray, TRUE, INFINITE);

    std::cout << "Waited for all objects" << std::endl;

    for (int i = 0; i < 8; i++) {
        std::cout << "Core: " << i << " ran " << fiberCount[i] << " fibers " << std::endl;
    }

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

    LPVOID localQueuePtr = TlsGetValue(queueTlsIndex); 

    // Have to call destructor manually 
    static_cast<Wado::Queue::ArrayQueue<void> *>(localQueuePtr)->~ArrayQueue();

    HeapFree(GetProcessHeap(), 0, TlsGetValue(queueTlsIndex));

    for (int i = 0; i < threadCount; i++) {
        CloseHandle(threadHandleArray[i]);
        if (threadAffinityData[i] != NULL) {
            HeapFree(GetProcessHeap(), 0, threadAffinityData[i]);
            threadAffinityData[i] = NULL; // Ensure address is not reused.
        };
    };

    HeapFree(GetProcessHeap(), 0, threadIDArray);
    HeapFree(GetProcessHeap(), 0, threadHandleArray);

    HeapFree(GetProcessHeap(), 0, globalQueue);

    //HeapFree(GetProcessHeap(), 0, systemLogicalInformationBuffer);

    TlsFree(queueTlsIndex);
    TlsFree(previousFiberTlsIndex);
    TlsFree(coreNumberTlsIndex);

    return 0;
};