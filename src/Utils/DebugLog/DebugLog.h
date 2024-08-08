#ifndef WADO_DEBUG_LOG_H
#define WADO_DEBUG_LOG_H

#include <cstdarg>
#include <cstdint>

namespace Wado::DebugLog { 

    enum WdLogSeverity {
        WD_NOTICE = 0,
        WD_MESSAGE = 1,
        WD_WARNING = 2,
        WD_ERROR = 3,
    };

    void DebugLogInit(size_t coreCount);

    void DebugLogShutdown();

    void DebugLogLocal(const WdLogSeverity severity, const char* systemName, const char* format, ...);

    void DebugLogGlobal(const WdLogSeverity severity, const char* systemName, const char* format, ...);

    // Returns the number of characters written to target
    // Caller must ensure that target is large enough to hold the formatted string
    // Supported formats: d - decimal, x - hexadecimal, b - binary, f - floating point, p - pointer
    size_t DebugMessageFormatter(char *target, const char* format, std::va_list args);

    #ifndef NDEBUG
    #define DEBUG_LOCAL(Severity, SystemName, MessageFormat, ...) DebugLogLocal(Severity, SystemName, MessageFormat, __VA_ARGS__);
    #define DEBUG_GLOBAL(Severity, SystemName, MessageFormat, ...) DebugLogGlobal(Severity, SystemName, MessageFormat, __VA_ARGS__); 
    #else
    #define DEBUG_LOCAL(Severity, SystemName, Message) {};
    #define DEBUG_GLOBAL(Severity, SystemName, Message) {}; 
    #endif
};

#endif