#include "TaskSystem.h"

#include <cstdlib>
#include <stdexcept>
#include <iostream>

namespace Wado::Task {
    void makeTask(Wado::Fiber::WdFiberFunctionPtr functionPtr, void *arguments, Wado::FiberSystem::WdFence* fenceToSignal) {
        WdFiberArgs* args = static_cast<WdFiberArgs *>(malloc(sizeof(WdFiberArgs)));

        if (args == nullptr) {
            throw std::runtime_error("Could not allocate space for task function arguments.");
        };

        Wado::FiberSystem::WdReadyQueueItem readyQueueItem = static_cast<Wado::FiberSystem::WdReadyQueueItem>(malloc(sizeof(Wado::Queue::Queue<void>::Item)));

        if (readyQueueItem == nullptr) {
            throw std::runtime_error("Could not create ready queue item for new task.");
        };

        args->mainArgs = arguments;
        args->fenceToSignal = fenceToSignal;
        args->readyQueueItem = readyQueueItem;

        // 64 kb stack 
        Wado::Fiber::WdFiberID fiberID = Wado::Fiber::WdCreateFiber(functionPtr, args);

        readyQueueItem->data = fiberID;
        readyQueueItem->node = nullptr;

        // push to global queue here if you cant to normal one

        Wado::Queue::Queue<void> *localReadyQueue = static_cast<Wado::Queue::Queue<void> *>(Wado::Thread::WdThreadLocalGetValue(TLlocalReadyQueueID));
        if (localReadyQueue->isFull()) {
            //std::cout << "Enqeueing on global queue" << std::endl;
            FiberGlobalReadyQueue.enqueue(readyQueueItem);
        } else {
            //std::cout << "Enqeueing on local queue" << std::endl;
            localReadyQueue->enqueue(readyQueueItem); 
        };    
    };

};