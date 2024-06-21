#ifndef WADO_PAL_SYSTEM_H
#define WADO_PAL_SYSTEM_H

#include <cstdint>

namespace Wado::System {
    using WdAvailableCoresMask = uint64_t;

    WdAvailableCoresMask WdGetAvailableSystemCores();
};

#endif