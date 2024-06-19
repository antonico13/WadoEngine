#include "Windows.h"

#include "Fiber.h"

#include <stdexcept>

namespace Wado::Fiber {

    WdFiberID WdStartFiber() {
        return ConvertThreadToFiber(NULL);
    };

    WdFiberID WdCreateFiber(WdFiberFunctionPtr function, void *fiberParam) {
        const size_t DEFAULT_FIBER_STACK = 65536;
        return CreateFiber(DEFAULT_FIBER_STACK, function, fiberParam);
    };

    WdFiberID WdGetCurrentFiber() {
        return GetCurrentFiber();
    };

    void WdSwitchFiber(WdFiberID fiber) {
        SwitchToFiber(fiber);
    };

    void WdDeleteFiber(WdFiberID fiber) {
        DeleteFiber(fiber);
    };

    WdFiberLocalID WdFiberLocalAllocate() {
        WdFiberLocalID newID = FlsAlloc(NULL);
        
        // if ((newID = FlsAlloc(NULL)) == FLS_OUT_OF_INDEXES) {
        //     throw std::runtime_error("Ran out of indices for fiber local storage");
        // };
        return newID;
    };

    void *WdFiberLocalGetValue(WdFiberLocalID valueID) {
        return FlsGetValue(valueID);
    };

    void WdFiberLocalSetValue(WdFiberLocalID valueID, void *value) {
        FlsSetValue(valueID, value);
        // if (!FlsSetValue(valueID, value)) {
        //     throw std::runtime_error("Could not set fiber local storage value");
        // };
    };

    void WdFiberLocalFree(WdFiberLocalID valueID) {
        FlsFree(valueID);
    };

};
