#ifndef WADO_PAL_SEMAPHORE_H
#define WADO_PAL_SEMAPHORE_H

#include <cstdint>

namespace Wado::Semaphore {
    using WdSemaphore = void *;

    WdSemaphore createSemaphore(uint32_t initialCount);

    void waitSemaphore(WdSemaphore semaphore, uint32_t count = 1);

    void releaseSemaphore(WdSemaphore semaphore, uint32_t count = 1);
};

#endif