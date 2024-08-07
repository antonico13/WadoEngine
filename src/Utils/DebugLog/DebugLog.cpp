#include "DebugLog.h"
#include "FileSys.h"
#include "Thread.h"

#include <string>
#include <chrono>

extern Wado::Thread::WdThreadLocalID TLcoreIndexID;

namespace Wado::DebugLog {
    // TODO: all of this is not very safe, need to add lots of checks for stuff such as null termination and sizes 

    // TODO: Const?
    static FileSystem::WdFileHandle localDebugLogHandles[sizeof(uint64_t) * 8];
    static FileSystem::WdFileHandle globalDebugLogHandle;
    static bool bDebugLogInit = false;

    static const char *prefix = "core_";
    static const char *postfix = ".txt";
    static const char *globalLog = "globalLog.txt";

    static const char *logSeverityMsg[] = {"Notice", "Message", "Warning", "Error"};

    static size_t _coreCount;

    static const size_t TIME_SIZE = 25;

    void DebugLogInit(size_t coreCount) {
        if (!bDebugLogInit) {
            _coreCount = coreCount;
            for (size_t i = 0; i < coreCount; ++i) {
                std::string localName = prefix + std::to_string(i) + postfix;
                localDebugLogHandles[i] = FileSystem::WdCreateFile(localName.c_str(), FileSystem::WdFileType::WD_READ | FileSystem::WdFileType::WD_WRITE);
            };

            globalDebugLogHandle = FileSystem::WdCreateFile(globalLog, FileSystem::WdFileType::WD_READ | FileSystem::WdFileType::WD_WRITE);
            
            bDebugLogInit = true;
        };
    }; 

    void DebugLogPrivate(const FileSystem::WdFileHandle debugStream, const WdLogSeverity severity, const char* systemName, const char* data) {
        std::chrono::system_clock::now();
        const std::time_t localTime = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
        std::string fullMessage;
        fullMessage += std::ctime(&localTime) + ': ';
        fullMessage += '[' + logSeverityMsg[severity] + '] ';
        fullMessage += '[' + systemName + '] ';
        fullMessage += data + '\n';

        FileSystem::WdWriteFile(debugStream, fullMessage.c_str(), fullMessage.size());
    };

    void DebugLogLocal(const WdLogSeverity severity, const char* systemName, const char* data) {
        DebugLogPrivate(Wado::Thread::WdThreadLocalGetValue(TLcoreIndexID), severity, systemName, data);
    };

    void DebugLogGlobal(const WdLogSeverity severity, const char* systemName, const char* data) {
        DebugLogPrivate(globalDebugLogHandle, severity, systemName, data);
    };

    void DebugLogShutdown() {
        if (bDebugLogInit) {
            for (size_t i = 0; i < _coreCount; ++i) {
                FileSystem::WdCloseFile(localDebugLogHandles[i]);
            };

            FileSystem::WdCloseFile(globalDebugLogHandle);

            bDebugLogInit = false;
        };
    };
};