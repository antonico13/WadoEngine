#ifndef WADO_GRAPHICS_ABSTRACTION_LAYER_BUFFER
#define WADO_GRAPHICS_ABSTRACTION_LAYER_BUFFER

#include "WdCommon.h"

namespace Wado::GAL {
    // Forward Declaration
    class WdLayer;

    using WdBufferHandle = WdHandle;

    using WdBufferUsageFlags = uint32_t;

    enum WdBufferUsage {
        WD_TRANSFER_SRC = 0x00000001,
        WD_TRANSFER_DST = 0x00000002,
        WD_UNIFORM_BUFFER = 0x00000004,
        WD_STORAGE_BUFFER = 0x00000008,
        WD_INDEX_BUFFER = 0x00000010,
        WD_VERTEX_BUFFER = 0x00000020,
        WD_UNIFORM_TEXEL_BUFFER = 0x00000040,
        WD_STORAGE_TEXEL_BUFFER = 0x00000080,
        WD_INDIRECT_BUFFER = 0x00000100,
    };

    using WdBuffer = struct WdBuffer {
        public:
            friend class WdLayer;
            //friend class Vulkan::VulkanLayer;

            const WdResourceID resourceID;

            const std::vector<WdBufferHandle> handles;
            const std::vector<WdMemoryHandle> memories;
            const std::vector<void *> dataPointers;
            const WdBufferUsageFlags usage;
            const WdSize size;
        private:
            WdBuffer(WdResourceID _resourceID, const std::vector<WdBufferHandle>& _handles, const std::vector<WdMemoryHandle>& _memories, WdSize _size, WdBufferUsageFlags _usage) :
                resourceID(_resourceID),
                handles(_handles),
                memories(_memories),
                usage(_usage),
                size(_size),
                dataPointers(handles.size()) {};
    };

    using WdBufferDescription = struct WdBufferDescription {
        WdBufferDescription(const WdBufferUsageFlags& _usage, const WdSize& _elementCount, const WdSize& _sizePerElement) : usage(_usage), elementCount(_elementCount), sizePerElement(_sizePerElement), size(_sizePerElement * _elementCount) { };
        const WdBufferUsageFlags usage;
        const WdSize elementCount;
        const WdSize sizePerElement;
        const WdSize size;
    };
}
#endif
