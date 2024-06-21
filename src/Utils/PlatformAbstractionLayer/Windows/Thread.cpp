#include "Windows.h"

#include "Thread.h"

#include <stdexcept>
#include <iostream>

namespace Wado::Thread {

    WdThreadHandle WdCreateThread(WdThreadStartFunctionPtr startFunction, size_t coreIndex) {
        // Always create thread in suspended state
        WdThreadHandle handle = CreateThread(NULL, 0, startFunction, (void *) coreIndex, CREATE_SUSPENDED, NULL);
        if (handle == nullptr)  {
           throw std::runtime_error("Could not create worker thread.");
        };
        return handle;
    };

    WdThreadHandle WdGetCurrentThreadHandle() {
        WdThreadHandle handle;
        if (!DuplicateHandle(GetCurrentProcess(), GetCurrentThread(), GetCurrentProcess(), &handle, NULL, FALSE, DUPLICATE_SAME_ACCESS)) {
            throw std::runtime_error("Could not get current worker thread handle.");
        };
        return handle;
    };

    void WdStartThread(WdThreadHandle threadHandle) {
        ResumeThread(threadHandle);
    };

    WdThreadLocalID WdThreadLocalAllocate() {
        WdThreadLocalID newID;

        if ((newID = TlsAlloc()) == TLS_OUT_OF_INDEXES) {
            throw std::runtime_error("Could not allocate thread local ID.");
        };
        return newID;
    };

    void *WdThreadLocalGetValue(WdThreadLocalID valueID) {
        void *value = TlsGetValue(valueID);
        if (GetLastError() != ERROR_SUCCESS) {
            throw std::runtime_error("Could not get thread local value");
        };
        return value;
    };

    void WdThreadLocalSetValue(WdThreadLocalID valueID, void *value) {
        if (!TlsSetValue(valueID, value)) {
            throw std::runtime_error("Could not set thread local value");
        };
    };

    void WdThreadLocalFree(WdThreadLocalID valueID) {
        TlsFree(valueID);
    };

    void WdSetThreadAffinity(WdThreadHandle threadHandle, size_t coreNumber) {
        DWORD_PTR threadAffinityMask = (DWORD_PTR)1 << coreNumber;
        DWORD_PTR oldThreadAffinity = SetThreadAffinityMask(threadHandle, threadAffinityMask);
        //std::cout << "Old thread affinity for thread " << 0 << " was " << std::bitset<sizeof(DWORD_PTR)>(oldThreadAffinity) << std::endl;

        if (oldThreadAffinity == 0) {
            throw std::runtime_error("Could not set thread affinity for worker thread.");
        }; 
    };

};