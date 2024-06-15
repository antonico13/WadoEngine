#ifndef WADO_ATOMICS_H
#define WADO_ATOMICS_H

#include <cstdint>

namespace Wado::Atomics {
    uint64_t TestAndSet(uint64_t volatile *target, uint64_t value);
    uint64_t Decrement(uint64_t volatile *target);
};

#endif