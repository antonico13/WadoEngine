#ifndef WADO_PAL_SYSTEM_H
#define WADO_PAL_SYSTEM_H

#include <cstdint>
#include <vector>

namespace Wado::System {

    using WdCoreInfo = struct WdCoreInfo {
        WdCoreInfo(const uint64_t number, const uint64_t neighbour) : coreNumber(number), hyperthreadLocalNeighbour(neighbour) { };
        const uint64_t coreNumber;
        const uint64_t hyperthreadLocalNeighbour;
    };

    // This uses ROV so it's fine, vector *SHOULD* fit in the return slot on x86-64 
    const std::vector<WdCoreInfo> WdGetAvailableSystemCores();
};

#endif