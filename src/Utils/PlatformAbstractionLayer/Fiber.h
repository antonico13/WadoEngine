#ifndef WADO_PAL_FIBER_H
#define WADO_PAL_FIBER_H

namespace Wado::Fiber {
    typedef void (*WdFiberFunctionPtr) (void *fiberParam);

    using WdFiberID = void *;
    using WdFiberLocalID = unsigned long;

    // Can only be called by a worker thread that hsn't become a fiber yet 
    WdFiberID WdStartFiber();
    WdFiberID WdCreateFiber(WdFiberFunctionPtr function, void *fiberParam);

    WdFiberID WdGetCurrentFiber();

    void WdSwitchFiber(WdFiberID fiber);

    void WdDeleteFiber(WdFiberID fiber);

    WdFiberLocalID WdThreadLocalAllocate();
    void *WdFiberLocalGetValue(WdFiberLocalID valueID);
    void WdFiberLocalSetValue(WdFiberLocalID valueID, void *value);
    void WdFiberLocalFree(WdFiberLocalID valueID);
};

#endif