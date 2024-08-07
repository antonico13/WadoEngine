#include "Windows.h"

#include "FileSys.h"

#include <stdexcept>

namespace Wado::FileSystem {

    const WdFileHandle WdCreateFile(const char *fileName, const WdFileTypeFlags fileType) {
        DWORD access = 0;

        if ((fileType & WdFileType::WD_READ) == WdFileType::WD_READ) {
            access |= GENERIC_READ;
        };

        if ((fileType & WdFileType::WD_WRITE) == WdFileType::WD_WRITE) {
            access |= GENERIC_WRITE;
        };

        HANDLE file = CreateFileA(fileName, access, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);

        if (file == INVALID_HANDLE_VALUE) {
            throw std::runtime_error("Could not create Windows file");
        };

        return file;
    };

    void WdCloseFile(const WdFileHandle fileHandle) {
        CloseHandle(fileHandle);
    };

    void WdWriteFile(const WdFileHandle fileHandle, const char *data, const size_t size) {
        DWORD bytesWritten = 0;
        BOOL res = WriteFile(fileHandle, data, size, &bytesWritten, NULL);

        if (res == FALSE) {
            throw std::runtime_error("Could not write to Windows file");
        };
    };
};