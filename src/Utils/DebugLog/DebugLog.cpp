#include "DebugLog.h"
#include "FileSys.h"
#include "Thread.h"

#include <string>
#include <chrono>

#include <iostream>

extern Wado::Thread::WdThreadLocalID TLcoreIndexID;

namespace Wado::DebugLog {
    // TODO: all of this is not very safe, need to add lots of checks for stuff such as null termination and sizes 

    // TODO: Const?
    static const size_t DEFAULT_BUFFER_SIZE = 1024;
    static FileSystem::WdFileHandle localDebugLogHandles[sizeof(size_t) * 8];
    static FileSystem::WdFileHandle globalDebugLogHandle;
    static char* localDebugLogBuffers[sizeof(size_t) * 8];
    static char* globalDebugLogBuffer;
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
                // TODO: remove the string from here, do C_STR only 
                std::string localName = prefix + std::to_string(i) + postfix;
                localDebugLogHandles[i] = FileSystem::WdCreateFile(localName.c_str(), FileSystem::WdFileType::WD_READ | FileSystem::WdFileType::WD_WRITE);
                
                // TODO: should these be malloc'ed or should I make them static?
                // I think malloc because with static init we could have 64kB of buffers just sitting here in the file, not that it really
                // matters for debug?
                localDebugLogBuffers[i] = static_cast<char *>(malloc(DEFAULT_BUFFER_SIZE));
                if (localDebugLogBuffers[i] == nullptr) {
                    throw std::runtime_error("Could not start the debug logger");
                };
            };

            globalDebugLogHandle = FileSystem::WdCreateFile(globalLog, FileSystem::WdFileType::WD_READ | FileSystem::WdFileType::WD_WRITE);
            globalDebugLogBuffer = static_cast<char *>(malloc(DEFAULT_BUFFER_SIZE));
            
            if (globalDebugLogBuffer == nullptr) {
                throw std::runtime_error("Could not start the debug logger");
            };

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

    // TODO: should add some constraints and stuff here for 
    // more type safety/ensuring a numeral 
    // Should do overloading for the other base cases instead of partial specialization
    // How do I do this in a single function here with template specialization?

    static const char HEX_REMAINDER[] = {'0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E', 'F'};
    static const char HEX_BITS_REQUIRED = 4;
    static const size_t HEX_REQUIRED_CHARS = sizeof(size_t) * 2;

    template <typename T>
    static char *itoa16(T number, char *target) {
        char *initialTarget = target;
        target += HEX_REQUIRED_CHARS;
        *(target--) = END_OF_MESSAGE;
        while (number > 0) {
            T quotient = number >> HEX_BITS_REQUIRED;
            T remainder = number - (quotient << HEX_BITS_REQUIRED);
            *(target--) = HEX_REMAINDER[remainder];
            number = quotient;
        };

        while (target >= initialTarget) {
            *(target--) = HEX_REMAINDER[0];
        };

        return target + HEX_REQUIRED_CHARS + 1;
    };

    static const char BINARY_REMAINDER[] = {'0', '1'};
    static const char BINARY_BITS_REQUIRED = 1;
    static const size_t BINARY_REQUIRED_CHARS = sizeof(size_t) * 8;


    template <typename T>
    static char *itoa2(T number, char *target) {
        char *initialTarget = target;
        target += BINARY_REQUIRED_CHARS;
        *(target--) = END_OF_MESSAGE;
        while (number > 0) {
            T quotient = number >> BINARY_BITS_REQUIRED;
            T remainder = number - (quotient << BINARY_BITS_REQUIRED);
            *(target--) = BINARY_REMAINDER[remainder];
            number = quotient;
        };

        while (target >= initialTarget) {
            *(target--) = BINARY_REMAINDER[0];
        };

        return target + BINARY_REQUIRED_CHARS + 1;
    };
    
    // TODO: should make this programatic too with MAX long long??
    static const size_t MAX_DECIMAL_DIGITS = 20;
    static const char ZERO = '0';

    template <typename T>
    static char *itoa10(T number, char *target) {
        if (number < 0) {
            *target = '-';
            ++target;
            number = -number;
        };

        char stack[MAX_DECIMAL_DIGITS];
        size_t digits = 0;
        do {
            T quotient = number / 10;
            T remainder = number - quotient * 10;
            stack[digits++] = ZERO + remainder;
            number = quotient;
        } while (number > 0); // do-while needed to handle 0 

        while (digits > 0) {
            *target = stack[digits - 1];
            ++target;
            --digits;
        };
        *(target + 1) = END_OF_MESSAGE;
        
        return target;
    };

    // TODO: Should I replace all the uints and stuff for portability?
    // TODO: lots of repetition here and above, need to look into smart templating 
    size_t DebugMessageFormatter(char *target, const char* format, std::va_list args) {
        size_t writtenCount = 0;
        char *oldTarget = nullptr;
        while (*format != END_OF_MESSAGE) {
            if (*format == FORMATTER) {
                ++format;
                switch (*format) {
                    case BINARY:
                        oldTarget = target;
                        *target = '0';
                        ++target;
                        *target = 'b';
                        ++target;

                        target = itoa2(va_arg(args, uint64_t), target);

                        writtenCount += (target - oldTarget);

                        break;
                    case DECIMAL:
                        oldTarget = target;
                        // TODO: why does int64_t bug out here?
                        target = itoa10(va_arg(args, int), target);

                        writtenCount += (target - oldTarget);

                        break;
                    case HEXADECIMAL:
                        oldTarget = target;
                        *target = '0';
                        ++target;
                        *target = 'x';
                        ++target;

                        target = itoa16(va_arg(args, uint64_t), target);

                        writtenCount += (target - oldTarget);

                        break;
                    // TODO: No floating point support yet 
                    case FLOATING_POINT:
                        break;
                    case POINTER:
                        oldTarget = target;
                        *target = '0';
                        ++target;
                        *target = 'p';
                        ++target;

                        target = itoa16(va_arg(args, uintptr_t), target);

                        writtenCount += (target - oldTarget);
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
                ++target;
                ++writtenCount;
            };
            ++format;
        };

        // Consider end of message as written also
        *target = END_OF_MESSAGE;
        ++writtenCount;

        return writtenCount;
    };

    // TODO: need to de-stringify this 
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

    void DebugLogLocal(const WdLogSeverity severity, const char* systemName, const char* format, ...) {
        size_t coreIndex = reinterpret_cast<size_t>(Wado::Thread::WdThreadLocalGetValue(TLcoreIndexID));
        std::va_list args;
        va_start(args, format);
        size_t writtenCount = DebugMessageFormatter(localDebugLogBuffers[coreIndex], format, args);
        va_end(args);
        DebugLogPrivate(localDebugLogHandles[coreIndex], severity, systemName, localDebugLogBuffers[coreIndex]);
    };

    void DebugLogGlobal(const WdLogSeverity severity, const char* systemName, const char* format, ...) {
        std::va_list args;
        va_start(args, format);
        size_t writtenCount = DebugMessageFormatter(globalDebugLogBuffer, format, args);
        va_end(args);
        DebugLogPrivate(globalDebugLogHandle, severity, systemName, globalDebugLogBuffer);
       };

    void DebugLogShutdown() {
        if (bDebugLogInit) {
            for (size_t i = 0; i < _coreCount; ++i) {
                FileSystem::WdCloseFile(localDebugLogHandles[i]);
                free(localDebugLogBuffers[i]);
            };

            FileSystem::WdCloseFile(globalDebugLogHandle);
            free(globalDebugLogBuffer);

            bDebugLogInit = false;
        };
    };
};