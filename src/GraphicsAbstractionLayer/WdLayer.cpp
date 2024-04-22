#include "WdLayer.h"

namespace Wado::GAL {
    
    WdImage* WdLayer::create2DImagePtr(WdImageHandle _handle, WdMemoryHandle _memory, WdRenderTarget _target, WdFormat _format, WdExtent3D _extent, WdImageUsageFlags _usage, WdClearValue _clearValue) {
        return new WdImage(_handle, _memory, _target, _format, _extent, _usage, _clearValue);
    };

    WdBuffer* WdLayer::createBufferPtr(WdBufferHandle _handle, WdMemoryHandle _memory, WdSize _size, WdBufferUsageFlags _usage) {
        return new WdBuffer(_handle, _memory, _size, _usage);
    };
};