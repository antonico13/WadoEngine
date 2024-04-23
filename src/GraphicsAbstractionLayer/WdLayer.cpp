#include "WdLayer.h"

namespace Wado::GAL {

    WdResourceID WdLayer::globalResourceID = 0;
    WdResourceID WdLayer::globalTempResourceID = TEMP_RESOURCE_BARRIER;

    WdImage* WdLayer::create2DImagePtr(const std::vector<WdImageHandle>& _handles, const std::vector<WdMemoryHandle>& _memories, const std::vector<WdRenderTarget>& _targets, WdFormat _format, WdExtent2D _extent, WdImageUsageFlags _usage, WdClearValue _clearValue) {
        globalResourceID++;
        return new WdImage(globalResourceID, _handles, _memories, _targets, _format, _extent, _usage, _clearValue);
    };

    WdBuffer* WdLayer::createBufferPtr(const std::vector<WdBufferHandle>& _handles, const std::vector<WdMemoryHandle>& _memories, WdSize _size, WdBufferUsageFlags _usage) {
        globalResourceID++;
        return new WdBuffer(globalResourceID, _handles, _memories, _size, _usage);
    };

    WdImage* WdLayer::createTemp2DImagePtr() {
        globalTempResourceID++;
        WdClearValue tempClearValue;
        tempClearValue.color = DEFAULT_COLOR_CLEAR;
        return new WdImage(globalTempResourceID, {}, {}, {}, WdFormat::WD_FORMAT_UNDEFINED, {0, 0}, 0, tempClearValue);
    };
    
    WdBuffer* WdLayer::createTempBufferPtr() {
        globalTempResourceID++;
        return new WdBuffer(globalTempResourceID, {}, {}, 0, 0);
    };
            
};