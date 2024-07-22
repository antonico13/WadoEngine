#include "Windows.h"

#include "Semaphore.h"

namespace Wado::Semaphore {
    
    WdSemaphore createSemaphore(uint32_t initialCount) {
        // Try to take advantage of RVO here by returning an R-value directly (I don't think void * could ever be an L-value since its a base type actually so it doesn't matter)
        return CreateSemaphoreA(NULL, 0, static_cast<LONG>(initialCount), NULL);
    };

    void waitSemaphore(WdSemaphore semaphore, uint32_t count) {
        // Windows semaphores are non signaled when the count is 0, signaled when > 0. 
        for (uint32_t i = 0; i < count; ++i) {
            WaitForSingleObject(static_cast<HANDLE>(semaphore), INFINITE); 
        };
    };

    void releaseSemaphore(WdSemaphore semaphore, uint32_t count) {
        ReleaseSemaphore(static_cast<HANDLE>(semaphore), static_cast<LONG>(count), NULL);
    };
};