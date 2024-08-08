#include <gtest/gtest.h>

#include "System.h"

#include "FiberSystem.h"
#include "TaskSystem.h"
#include "DebugLog.h"

#include <cstdlib>
#include <iostream>
#include <vector>

Wado::FiberSystem::WdLock globalLock;

size_t fiberCount = 0;

size_t coreStart[sizeof(uint64_t) * 8];
size_t coreEnd[sizeof(uint64_t) * 8];

volatile size_t count = 50;

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
    DEBUG_LOCAL(Wado::DebugLog::WD_MESSAGE, "FiberTests", "Finished Next Task");
    globalLock.acquire();
    size_t endCore = (size_t) Wado::Thread::WdThreadLocalGetValue(TLcoreIndexID);
    coreEnd[endCore]++;
    fiberCount++;
    globalLock.release();
    DEBUG_LOCAL(Wado::DebugLog::WD_MESSAGE, "FiberTests", "Fiber number: %d as a dummy task", fiberCount);
    DEBUG_LOCAL(Wado::DebugLog::WD_MESSAGE, "FiberTests", "Started on core: %d, finished on: %d", startCore, endCore);
    DEBUG_LOCAL(Wado::DebugLog::WD_MESSAGE, "FiberTests", "This many fibers also started on this core: %d", coreStart[startCore]);
    DEBUG_LOCAL(Wado::DebugLog::WD_MESSAGE, "FiberTests", "This many fibers also finished on this core: %d", coreEnd[endCore]);
});

TASK(DummyTask, DummyData, data, {
    size_t startCore = (size_t) Wado::Thread::WdThreadLocalGetValue(TLcoreIndexID);
    coreStart[startCore]++;
    Wado::Atomics::Decrement(&count);
    DEBUG_LOCAL(Wado::DebugLog::WD_MESSAGE, "FiberTests", "Finished Dummy Task");
    globalLock.acquire();
    size_t endCore = (size_t) Wado::Thread::WdThreadLocalGetValue(TLcoreIndexID);
    coreEnd[endCore]++;
    fiberCount++;
    globalLock.release();
    DEBUG_LOCAL(Wado::DebugLog::WD_MESSAGE, "FiberTests", "Fiber number: %d as a dummy task", fiberCount);
    DEBUG_LOCAL(Wado::DebugLog::WD_MESSAGE, "FiberTests", "Started on core: %d, finished on: %d", startCore, endCore);
    DEBUG_LOCAL(Wado::DebugLog::WD_MESSAGE, "FiberTests", "This many fibers also started on this core: %d", coreStart[startCore]);
    DEBUG_LOCAL(Wado::DebugLog::WD_MESSAGE, "FiberTests", "This many fibers also finished on this core: %d", coreEnd[endCore]);
    Wado::Task::makeTask(NextTask, nullptr, data->nextFence);
});

static void FiberTest(const size_t coreCount) {
    Wado::FiberSystem::WdFence *fence = static_cast<Wado::FiberSystem::WdFence *>(malloc(sizeof(Wado::FiberSystem::WdFence)));
    Wado::FiberSystem::WdFence *nextFence = static_cast<Wado::FiberSystem::WdFence *>(malloc(sizeof(Wado::FiberSystem::WdFence)));

    if ( (fence == nullptr) || (nextFence == nullptr) ) {
        std::cout << "Could not allocate fence" << std::endl;
    };

    size_t taskCount = count / 2;

    new (fence) Wado::FiberSystem::WdFence(taskCount);
    new (nextFence) Wado::FiberSystem::WdFence(taskCount);

    DEBUG_GLOBAL(Wado::DebugLog::WD_MESSAGE, "FiberTests", "Made fences");

    DummyData data;
    data.x = 0;
    data.y = 1;
    data.nextFence = nextFence;

    for (size_t i = 0; i < taskCount; i++) {
        Wado::Task::makeTask(DummyTask, &data, fence);
    };

    DEBUG_GLOBAL(Wado::DebugLog::WD_MESSAGE, "FiberTests", "Made tasks");

    fence->waitForSignal();

    nextFence->waitForSignal();
    
    DEBUG_GLOBAL(Wado::DebugLog::WD_MESSAGE, "FiberTests", "Finished waiting");
    
    fence->~WdFence();
    nextFence->~WdFence();

    free(fence);
    free(nextFence);

    for (size_t i = 0; i < coreCount; ++i) {
        DEBUG_GLOBAL(Wado::DebugLog::WD_MESSAGE, "FiberTests", "Core: %d started %d fibers", i, coreStart[i]);
        DEBUG_GLOBAL(Wado::DebugLog::WD_MESSAGE, "FiberTests", "Core: %d unlocked %d fibers", i, coreEnd[i]);
    };

    DEBUG_GLOBAL(Wado::DebugLog::WD_MESSAGE, "FiberTests", "Total fibers: %d", fiberCount);
    DEBUG_GLOBAL(Wado::DebugLog::WD_MESSAGE, "FiberTests", "Shutting down");
    DEBUG_GLOBAL(Wado::DebugLog::WD_MESSAGE, "FiberTests", "Count value: %d", count);
};

// TEST(InitFiberTest, UsesAllCoresByDefault) {
//     const std::vector<Wado::System::WdCoreInfo> coreInfos = Wado::FiberSystem::InitFiberSystem();
//     ASSERT_EQ(coreInfos.size(), Wado::System::WdGetAvailableSystemCores().size()) << "Expected the utilised core number to be the system max core number";
//     Wado::FiberSystem::ShutdownFiberSystem();
// };


TEST(InitFiberTest, UsesRequestedCoreAmount) {
    const std::vector<Wado::System::WdCoreInfo> coreInfos = Wado::FiberSystem::InitFiberSystem(3);
    ASSERT_EQ(coreInfos.size(), 3) << "Expected the utilised core number to be the requested core number";
    FiberTest(coreInfos.size());
    Wado::FiberSystem::ShutdownFiberSystem();
};

// TEST(InitFiberTest, PrioritisesHyperthreading) {
//     const std::vector<Wado::System::WdCoreInfo> coreInfos = Wado::FiberSystem::InitFiberSystem(5);

//     uint64_t firstNeighbour = coreInfos[0].hyperthreadLocalNeighbour;
//     ASSERT_EQ(coreInfos[firstNeighbour].hyperthreadLocalNeighbour, 0) << "Expected the neighbour of the first core's neighbour to be the first core";

//     uint64_t secondNeighbour = coreInfos[2].hyperthreadLocalNeighbour;
//     ASSERT_EQ(coreInfos[secondNeighbour].hyperthreadLocalNeighbour, 2) << "Expected the neighbour of the third core's neighbour to be the third core";

//     ASSERT_GE(coreInfos[4].hyperthreadLocalNeighbour, coreInfos.size()) << "Expected the neighbour of the odd core to not be in the returned core info set";

//     Wado::FiberSystem::ShutdownFiberSystem();
// };


