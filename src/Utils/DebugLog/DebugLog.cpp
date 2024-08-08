#include "DebugLog.h"
#include "FileSys.h"
#include "Thread.h"

#include <string>
#include <chrono>

#include <stdlib.h>

#include <iostream>

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

    static const char END_OF_MESSAGE = '\0';
    static const char FORMATTER = '%';
    static const char BINARY = 'b';
    static const char DECIMAL = 'd';
    static const char HEXADECIMAL = 'x';
    static const char FLOATING_POINT = 'f';
    static const char POINTER = 'p';
    static const char WHITESPACE = ' ';
    
    static const size_t BYTE_COUNT = sizeof(int);

    uint64_t DebugMessageFormatter(char *target, const char* format, std::va_list args) {
        va_start(args, format);
        uint64_t writtenCount = 0;
        while (*format != END_OF_MESSAGE) {
            if (*format == FORMATTER) {
                ++format;
                switch (*format) {
                    case BINARY:
                        *target = '0';
                        ++target;
                        *target = 'b';
                        ++target;

                        int i = va_arg(args, int);
                        itoa(i, target, 2);

                        
                        while (target != END_OF_MESSAGE) {
                            ++writtenCount;
                            ++target;
                        };

                        break;
                    case DECIMAL:
                        int i = va_arg(args, int);
                        itoa(i, target, 10);
                        
                        while (target != END_OF_MESSAGE) {
                            ++writtenCount;
                            ++target;
                        };

                        break;
                    case HEXADECIMAL:
                        *target = '0';
                        ++target;
                        *target = 'x';
                        ++target;

                        int i = va_arg(args, int);
                        itoa(i, target, 16);

                        
                        while (target != END_OF_MESSAGE) {
                            ++writtenCount;
                            ++target;
                        };

                        break;
                    case FLOATING_POINT:
                        break;
                    case POINTER:
                        *target = '0';
                        ++target;
                        *target = 'p';
                        ++target;

                        uintptr_t i = va_arg(args, uintptr_t);
                        itoa(i, target, 16);

                        
                        while (target != END_OF_MESSAGE) {
                            ++writtenCount;
                            ++target;
                        };
                        break;
                    case FORMATTER:
                        *target = FORMATTER;
                        ++target;
                        ++writtenCount;
                        break;
                    default:
                        *target = WHITESPACE;
                        ++target;
                        ++writtenCount;
                        break;
                };
            } else {
                *target = *format;
                writtenCount++;
            };
            ++format;
        };

        *target = END_OF_MESSAGE;

        return writtenCount;
    };


    void DebugLogPrivate(const FileSystem::WdFileHandle debugStream, const WdLogSeverity severity, const char* systemName, const char* data) {
        // TODO: this is very, very ugly. Might be easier to do with buffers directly
        // than all this string manipulation and iterators 
        const std::time_t localTime = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
        std::string fullMessage;
        fullMessage += std::ctime(&localTime);
        fullMessage.erase((++fullMessage.rbegin()).base());
        fullMessage += ": ";
        fullMessage += "[";
        fullMessage += logSeverityMsg[severity];
        fullMessage += "] ";
        fullMessage += "[";
        fullMessage += systemName;
        fullMessage += "] ";
        fullMessage += data;
        fullMessage += "\n";

        FileSystem::WdWriteFile(debugStream, fullMessage.c_str(), fullMessage.size());
    };

    void DebugLogLocal(const WdLogSeverity severity, const char* systemName, const char* data) {
        DebugLogPrivate(localDebugLogHandles[reinterpret_cast<size_t>(Wado::Thread::WdThreadLocalGetValue(TLcoreIndexID))], severity, systemName, data);
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