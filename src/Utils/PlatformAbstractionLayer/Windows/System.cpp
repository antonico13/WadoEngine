#include "Windows.h"

#include "System.h"

#include <stdexcept>

namespace Wado::System {
    WdAvailableCoresMask WdGetAvailableSystemCores() {
        HANDLE currentProcess = GetCurrentProcess();
        DWORD_PTR processAffinityMask;
        DWORD_PTR systemAffinityMask;
        BOOL res;
        res = GetProcessAffinityMask(currentProcess, &processAffinityMask, &systemAffinityMask);

        if (res == FALSE) {
            throw std::runtime_error("Could not get system information");
        };

        return static_cast<WdAvailableCoresMask>(processAffinityMask);
    };
};