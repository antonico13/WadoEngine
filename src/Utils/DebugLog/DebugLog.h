#ifndef WADO_DEBUG_LOG_H
#define WADO_DEBUG_LOG_H

namespace Wado::DebugLog { 

    enum WdLogSeverity {
        WD_NOTICE = 0,
        WD_MESSAGE = 1,
        WD_WARNING = 2,
        WD_ERROR = 3,
    };

    void DebugLogInit(size_t coreCount);

    void DebugLogShutdown();

    void DebugLogLocal(const WdLogSeverity severity, const char* systemName, const char* data);

    void DebugLogGlobal(const WdLogSeverity severity, const char* systemName, const char* data);

    #define DEBUG_LOCAL(Severity, SystemName, Message) \
        #ifndef NDEBUG \
            DebugLogLocal(Severity, SystemName, Message); \
        #endif

    #define DEBUG_GLOBAL(Severity, SystemName, Message) \
        #ifndef NDEBUG \
            DebugLogGlobal(Severity, SystemName, Message); \
        #endif
};

#endif