#include <gtest/gtest.h>

#include "System.h"

#include "FiberSystem.h"
#include "TaskSystem.h"

#include <cstdlib>
#include <iostream>
#include <vector>

Wado::FiberSystem::WdLock globalLock;

size_t fiberCount = 0;

size_t coreStart[8];
size_t coreEnd[8];

volatile size_t count = 800;

typedef struct DummyData {
    int x; 
    int y;
    Wado::FiberSystem::WdFence* nextFence;
} DummyData;

typedef struct NextData {

} NextData;

TASK(NextTask, NextData, data, {
    size_t startCore = (size_t) Wado::Thread::WdThreadLocalGetValue(TLcoreIndexID);
    coreStart[startCore]++;
    Wado::Atomics::Decrement(&count);
    globalLock.acquire();
    size_t endCore = (size_t) Wado::Thread::WdThreadLocalGetValue(TLcoreIndexID);
    coreEnd[endCore]++;
    fiberCount++;
    std::cout << "Fiber number: " << fiberCount << " as a dummy task" << std::endl;
    std::cout << "Started on core: " << startCore << " finished on " << endCore << std::endl;
    std::cout << "This many fibers also started on this core: " << coreStart[startCore] << std::endl;
    std::cout << "This many fibers also finished on this core: " << coreEnd[endCore] << std::endl;
    globalLock.release();
});

TASK(DummyTask, DummyData, data, {
    size_t startCore = (size_t) Wado::Thread::WdThreadLocalGetValue(TLcoreIndexID);
    coreStart[startCore]++;
    Wado::Atomics::Decrement(&count);
    globalLock.acquire();
    size_t endCore = (size_t) Wado::Thread::WdThreadLocalGetValue(TLcoreIndexID);
    coreEnd[endCore]++;
    fiberCount++;
    std::cout << "Fiber number: " << fiberCount << " as a dummy task" << std::endl;
    std::cout << "Started on core: " << startCore << " finished on " << endCore << std::endl;
    std::cout << "This many fibers also started on this core: " << coreStart[startCore] << std::endl;
    std::cout << "This many fibers also finished on this core: " << coreEnd[endCore] << std::endl;
    globalLock.release();
    Wado::Task::makeTask(NextTask, nullptr, data->nextFence);
});

static void FiberTest() {
    Wado::FiberSystem::WdFence *fence = static_cast<Wado::FiberSystem::WdFence *>(malloc(sizeof(Wado::FiberSystem::WdFence)));
    Wado::FiberSystem::WdFence *nextFence = static_cast<Wado::FiberSystem::WdFence *>(malloc(sizeof(Wado::FiberSystem::WdFence)));

    if ( (fence == nullptr) || (nextFence == nullptr) ) {
        std::cout << "Could not allocate fence" << std::endl;
    };

    size_t taskCount = count / 2;

    new (fence) Wado::FiberSystem::WdFence(taskCount);
    new (nextFence) Wado::FiberSystem::WdFence(taskCount);

    std::cout << "Made fences" << std::endl;

    DummyData data;
    data.x = 0;
    data.y = 1;
    data.nextFence = nextFence;

    for (size_t i = 0; i < taskCount; i++) {
        Wado::Task::makeTask(DummyTask, &data, fence);
    };

    std::cout << "Made tasks" << std::endl;

    fence->waitForSignal();

    nextFence->waitForSignal();
    
    std::cout << "Finished waiting" << std::endl;

    fence->~WdFence();
    nextFence->~WdFence();

    free(fence);
    free(nextFence);

    for (int i = 0; i < 8; i++) {
        std::cout << "Core: " << i << " started " << coreStart[i] << " fibers " << std::endl;
        std::cout << "Core: " << i << " unlocked " << coreEnd[i] << " fibers " << std::endl;
    };
    std::cout << "Total fibers: " << fiberCount << std::endl;

    std::cout << "Shutting down " << std::endl;
    
    std::cout << "Count value: " << count << std::endl; 
};

// TEST(InitFiberTest, UsesAllCoresByDefault) {
//     const std::vector<Wado::System::WdCoreInfo> coreInfos = Wado::FiberSystem::InitFiberSystem();
//     ASSERT_EQ(coreInfos.size(), Wado::System::WdGetAvailableSystemCores().size());
//     Wado::FiberSystem::ShutdownFiberSystem();
// };


TEST(InitFiberTest, UsesRequestedCoreAmount) {
    const std::vector<Wado::System::WdCoreInfo> coreInfos = Wado::FiberSystem::InitFiberSystem(3);
    ASSERT_EQ(coreInfos.size(), 3);
    Wado::FiberSystem::ShutdownFiberSystem();
};
