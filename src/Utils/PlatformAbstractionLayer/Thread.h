#ifndef WADO_PAL_THREAD_H
#define WADO_PAL_THREAD_H

namespace Wado::Thread {
    // todo: should this be std call?
    typedef unsigned long (*WdThreadStartFunctionPtr) (void *threadParam);

    using WdThreadLocalID = unsigned long;
    using WdThreadHandle = void *;

    // Always creates a thread in suspended mode 
    WdThreadHandle WdCreateThread(WdThreadStartFunctionPtr startFunction, void *threadParam);
    void WdStartThread(WdThreadHandle threadHandle);

    WdThreadLocalID WdThreadLocalAllocate();
    void *WdThreadLocalGetValue(WdThreadLocalID valueID);
    void WdThreadLocalSetValue(WdThreadLocalID valueID, void *value);
    void WdThreadLocalFree(WdThreadLocalID valueID);

    void WdSetThreadAffinity(size_t coreNumber);

};

#endif