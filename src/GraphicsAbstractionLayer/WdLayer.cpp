#include "WdLayer.h"

namespace Wado::GAL {

    uint32_t WdLayer::globalResourceID = 0;

    WdImage* WdLayer::create2DImagePtr(const std::vector<WdImageHandle>& _handles, const std::vector<WdMemoryHandle>& _memories, const std::vector<WdRenderTarget>& _targets, WdFormat _format, WdExtent2D _extent, WdImageUsageFlags _usage, WdClearValue _clearValue) {
        globalResourceID++;
        return new WdImage(globalResourceID, _handles, _memories, _targets, _format, _extent, _usage, _clearValue);
    };

    WdBuffer* WdLayer::createBufferPtr(const std::vector<WdBufferHandle>& _handles, const std::vector<WdMemoryHandle>& _memories, WdSize _size, WdBufferUsageFlags _usage) {
        globalResourceID++;
        return new WdBuffer(globalResourceID, _handles, _memories, _size, _usage);
    };
};