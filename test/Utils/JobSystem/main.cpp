#include <windows.h>

#include <iostream>
#include <bitset>

#include <stdexcept>

#include "ArrayQueue.h"
#include "LinkedListQueue.h"

typedef struct ThreadAffinityData {
    DWORD_PTR threadAffinity;
} THREADAFFINITYDATA, *PTHREADAFFINITYDATA;

typedef PTHREADAFFINITYDATA *PPTHREADAFFINITYDATA;

DWORD queueTlsIndex;
DWORD previousFiberTlsIndex;
DWORD coreNumberTlsIndex;

typedef struct FiberData {
    LPVOID fiberPtr;
} FiberData;

DWORD WINAPI ThreadAffinityTestFunction(LPVOID lpParam) {
    PTHREADAFFINITYDATA affinityData = (PTHREADAFFINITYDATA) lpParam;
    DWORD_PTR threadAffinity = affinityData->threadAffinity;
    DWORD64 coreNumber = 0;

    while (threadAffinity > 0) {
        coreNumber++;
        threadAffinity = threadAffinity >> 1;
    };

    std::cout << "Running thread on core:" << threadAffinity << std::endl;

    if (!TlsSetValue(coreNumberTlsIndex, (LPVOID) coreNumber)) {
        throw std::runtime_error("Could not set tls core numebr value");
    };

    LPVOID lpvQueue; 
    lpvQueue = (LPVOID) HeapAlloc(GetProcessHeap(), 0, sizeof(Wado::Queue::ArrayQueue<void>));

    if (lpvQueue == NULL) {
        throw std::runtime_error("Could not allocate memory for local queues");
    };

    if (!TlsSetValue(queueTlsIndex, lpvQueue)) {
        throw std::runtime_error("Could not set tls value");
    };

    LPVOID fiberID = ConvertThreadToFiber(NULL);

    // Now, we are a fiber, do main fiber loop 
    std::cout << "Became fiber" << std::endl;

    HeapFree(GetProcessHeap(), 0, TlsGetValue(queueTlsIndex));

    return 0;
};

void FiberYield() {
    Wado::Queue::Queue<void> *localReadyQueue = static_cast<Wado::Queue::Queue<void> *>(TlsGetValue(queueTlsIndex));
    if ((localReadyQueue == nullptr) && (GetLastError() != ERROR_SUCCESS)) {
        throw std::runtime_error("Could not get local ready queue");
    };
    
    while (localReadyQueue->isEmpty()) { };
    
    LPVOID nextFiber = static_cast<LPVOID>(localReadyQueue->dequeue());
    if (!TlsSetValue(previousFiberTlsIndex, GetCurrentFiber())) {
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
                localReadyQueue->enqueue(waiters.dequeue());
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

class Fence {
    public:
        Fence() {
            makeFenceData();
        };

        ~Fence() noexcept {
            // assuming its only destroyed when no waiters
            free(currentFenceData);
        };

        void reset() noexcept {
            free(currentFenceData);
            makeFenceData();
        };

        // I don't think this actually does anything 
        void signal() noexcept {
            PVOID volatile previousFenceData; 
            InterlockedExchangePointer(&previousFenceData, currentFenceData);
            FenceData* fenceData = static_cast<FenceData *>(previousFenceData);
            InterlockedExchange(&fenceData->fence, _signaled);
            Wado::Queue::Queue<void> *localReadyQueue = static_cast<Wado::Queue::Queue<void> *>(TlsGetValue(queueTlsIndex));
            while (!fenceData->waiters.isEmpty()) {
                localReadyQueue->enqueue(fenceData->waiters.dequeue());
            };
        }

        void waitForSignal() {
            if (!InterlockedAnd(&currentFenceData->fence, _signaled)) {
                currentFenceData->waiters.enqueue(GetCurrentFiber());
                FiberYield();
            };
        };
    private:

        void makeFenceData() {
            currentFenceData = static_cast<FenceData *>(malloc(sizeof(FenceData)));

            if (currentFenceData == nullptr) {
                throw std::runtime_error("Could not create new fence data");
            };

            currentFenceData->fence = _unsignaled;
            currentFenceData->waiters = Wado::Queue::LinkedListQueue<void>();
        };

        const LONG _unsignaled = 0;
        const LONG _signaled = 1;

        using FenceData = struct FenceData {
            LONG volatile fence;
            // This is kinda wild 
            Wado::Queue::LinkedListQueue<void> waiters;
        };

        FenceData* currentFenceData;
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
        if ((previousFiberPtr == 0) && (GetLastError() != ERROR_SUCCESS)) {\
            throw std::runtime_error("Could not get previous task pointer");\
        };\
        DeleteFiber(previousFiberPtr);\
        fiberArgs *args = static_cast<fiberArgs *>(fiberParam);\
        ArgumentType *ArgumentName = static_cast<ArgumentType *>(args->mainArgs);\
        Code;\
        Wado::Queue::Queue<void> *localReadyQueue = static_cast<Wado::Queue::Queue<void> *>(TlsGetValue(queueTlsIndex));\
        if ((localReadyQueue == nullptr) && (GetLastError() != ERROR_SUCCESS)) {\
            throw std::runtime_error("Could not get local ready queue");\
        };\
        if (args->fenceToSignal != 0) {\
            Fence *fence = static_cast<Fence *>(args->fenceToSignal);\
            fence->signal();\
        };\
        free(args);\
        while (localReadyQueue->isEmpty()) { };\
        LPVOID nextFiber = static_cast<LPVOID>(localReadyQueue->dequeue());\
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

TASK(DummyTask, DummyData, data, {
    std::cout << "X: " << data->x << std::endl;
    std::cout << "Y: " << data->y << std::endl;
});

// Arguments and fence memory is managed by the caller of this
void makeTask(LPFIBER_START_ROUTINE function, void *arguments, Fence* fenceToSignal = nullptr) {
    fiberArgs* args = static_cast<fiberArgs *>(malloc(sizeof(fiberArgs)));
    if (args == nullptr) {
        throw std::runtime_error("Could not allocate space for function arguments");
    };

    args->mainArgs = arguments;
    args->fenceToSignal = fenceToSignal;

    LPVOID fiberPtr = CreateFiber(0, function, args);

    // push to global queue here if you cant to normal one

    Wado::Queue::Queue<void> *localReadyQueue = static_cast<Wado::Queue::Queue<void> *>(TlsGetValue(queueTlsIndex));
    localReadyQueue->enqueue(fiberPtr);
};


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

    for (int i = 1; i < threadCount; i++) {
        ResumeThread(threadHandleArray[i]);
    };

    std::cout << "Running main thread on core: " << threadAffinityData[0]->threadAffinity << std::endl;

    // Now, convert this thread to a fiber 

    LPVOID fiberID = ConvertThreadToFiber(NULL);

    // Now, we are a fiber, do main fiber loop 
    std::cout << "Main thread became fiber" << std::endl;

    Sleep(5000);
    
    //WaitForMultipleObjects(threadCount, threadHandleArray, TRUE, INFINITE);

    std::cout << "Waited for all objects" << std::endl;

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

    for (int i = 0; i < threadCount; i++) {
        CloseHandle(threadHandleArray[i]);
        if (threadAffinityData[i] != NULL) {
            HeapFree(GetProcessHeap(), 0, threadAffinityData[i]);
            threadAffinityData[i] = NULL;    // Ensure address is not reused.
        };
    };

    HeapFree(GetProcessHeap(), 0, threadIDArray);
    HeapFree(GetProcessHeap(), 0, threadHandleArray);

    //HeapFree(GetProcessHeap(), 0, systemLogicalInformationBuffer);

    TlsFree(queueTlsIndex);
    TlsFree(previousFiberTlsIndex);

    return 0;
};