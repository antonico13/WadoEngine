#include "Windows.h"

#include "Atomics.h"

namespace Wado::Atomics {
    uint64_t TestAndSet(uint64_t volatile *target, uint64_t value) {
        return InterlockedExchange(target, value);
    };

    uint64_t Decrement(uint64_t volatile *target) {
        return InterlockedDecrement(target);
    };    
};