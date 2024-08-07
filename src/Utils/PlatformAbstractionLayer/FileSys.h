#ifndef WADO_FILESYS_H
#define WADO_FILESYS_H

#include <cstdint>

namespace Wado::FileSystem {
    using WdFileHandle = void *;

    enum WdFileType {
        WD_READ = 0x01,
        WD_WRITE = 0x02,
    };

    using WdFileTypeFlags = uint64_t;

    const WdFileHandle WdCreateFile(const char *fileName, const WdFileTypeFlags fileType);
    void WdCloseFile(const WdFileHandle fileHandle);
    void WdWriteFile(const WdFileHandle fileHandle, const char *data, const size_t size);
};

#endif