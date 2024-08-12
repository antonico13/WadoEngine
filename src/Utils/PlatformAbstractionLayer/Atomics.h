#ifndef WADO_ATOMICS_H
#define WADO_ATOMICS_H

#include <cstdint>

namespace Wado::Atomics {
    uint64_t TestAndSet(uint64_t volatile *target, uint64_t value);
    uint64_t Decrement(uint64_t volatile *target);
    void *TestAndSetPointer(void * volatile *target, void *value);
    void *CompareAndSwapPointer(void * volatile *destination, void *exchange, void *comperand);
    uint64_t CompareAndSwap(uint64_t volatile *target, uint64_t exchange, uint64_t comperand);
};

#endif