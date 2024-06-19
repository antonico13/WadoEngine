#include <windows.h>

#include <iostream>

#include "tracy/Tracy.hpp"
#include "common/TracySystem.hpp"

Wado::FiberSystem::WdLock globalLock;

size_t fiberCount = 0;

size_t coreStart[8];
size_t coreEnd[8];

volatile size_t count = 1600;

typedef struct DummyData {
    int x; 
    int y;
    Fence* sillyFence;
} DummyData;

typedef struct SillyData {

} SillyData;

TASK(NextTask, NextData, data, {
    { ZoneScopedNS("NextTask", 5);
    size_t startCore = (size_t) Wado::Thread::WdThreadLocalGetValue(TLcoreIndexID);
    coreStart[startCore]++;
    Wado::Atomics::Decrement(&count); 
    }
    globalLock.acquire();
    { ZoneScopedN("Critical Section Next");
    size_t endCore = (size_t) Wado::Thread::WdThreadLocalGetValue(TLcoreIndexID);
    coreEnd[endCore]++;
    fiberCount++;
    //std::coutut << "Fiber number: " << fiberCount << " as a dummy task" << std::endl;
    //std::coutut << "Started on core: " << startCore << " finished on " << endCore << std::endl;
    //std::coutut << "This many fibers also started on this core: " << coreStart[startCore] << std::endl;
    //std::coutut << "This many fibers also finished on this core: " << coreEnd[endCore] << std::endl;
    globalLock.release(); 
    }
});

TASK(DummyTask, DummyData, data, {
    { ZoneScopedNS("DummyTaskEntry", 5);
    size_t startCore = (size_t) Wado::Thread::WdThreadLocalGetValue(TLcoreIndexID);
    coreStart[startCore]++;
    Wado::Atomics::Decrement(&count);
    }
    globalLock.acquire();
    {ZoneScopedN("CriticalSection");
    size_t endCore = (size_t) Wado::Thread::WdThreadLocalGetValue(TLcoreIndexID);
    coreEnd[endCore]++;
    fiberCount++;
    //std::coutut << "Fiber number: " << fiberCount << " as a dummy task" << std::endl;
    //std::coutut << "Started on core: " << startCore << " finished on " << endCore << std::endl;
    //std::coutut << "This many fibers also started on this core: " << coreStart[startCore] << std::endl;
    //std::coutut << "This many fibers also finished on this core: " << coreEnd[endCore] << std::endl;
    globalLock.release(); 
    };
    Wado::Task::makeTask(NextTask, nullptr, data->nextFence);
});

int main(int argc, char** argv) {
    int i = 0;
    while (i < (1<<30)) {
        i++; // spin for tracy 
    };

    Wado::FiberSystem::InitFiberSystem();
    
    //FrameMarkStart(frameName);
    Wado::FiberSystem::WdFence *fence = static_cast<Wado::FiberSystem::WdFence *>(malloc(sizeof(Wado::FiberSystem::WdFence)));
    Wado::FiberSystem::WdFence *nextFence = static_cast<Wado::FiberSystem::WdFence *>(malloc(sizeof(Wado::FiberSystem::WdFence)));

    if ( (fence == nullptr) || (nextFence == nullptr) ) {
        //std::coutut << "Could not allocate fence" << std::endl;
    };

    std::cout << "Making 800 silly fences" << std::endl;

    Fence *sillyFence = static_cast<Fence *>(HeapAlloc(GetProcessHeap(), 0, sizeof(Fence)));

    if (sillyFence == nullptr) {
        std::cout << "Could not allocate silly fence" << std::endl;
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
    
    const size_t taskCount = 800;

    new (fence) Wado::FiberSystem::WdFence(taskCount);
    new (nextFence) Wado::FiberSystem::WdFence(taskCount);

    //std::coutut << "Made fences" << std::endl;

    DummyData data;
    data.x = 0;
    data.y = 1;
    data.sillyFence = sillyFence;

    for (size_t i = 0; i < taskCount; i++) {
        Wado::Task::makeTask(DummyTask, &data, fence);
    }; 
    

    //std::coutut << "Made tasks" << std::endl;

    fence->waitForSignal();

    nextFence->waitForSignal();
    //std::coutut << "Finished waiting" << std::endl;

    fence->~WdFence();
    nextFence->~WdFence();

    free(fence);
    free(nextFence);

    for (int i = 0; i < 8; i++) {
        std::cout << "Core: " << i << " started " << fiberCount[i] << " fibers " << std::endl;
        std::cout << "Core: " << i << " unlocked " << unlockCount[i] << " fibers " << std::endl;
    };
    std::cout << "Total fibers: " << fiberCount << std::endl;

    std::cout << "Shutting down " << std::endl;
    
    std::cout << "Count value: " << count << std::endl;
    //FrameMarkEnd(frameName);
    FrameMark; 
    return 0;

    Wado::FiberSystem::ShutdownFiberSystem();
};