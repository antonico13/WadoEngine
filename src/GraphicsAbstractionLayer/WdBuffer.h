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

            const WdBufferHandle handle;
            const WdMemoryHandle memory;
            const WdBufferUsageFlags usage;
            const WdSize size;
        private:
            WdBuffer(WdBufferHandle _handle, WdMemoryHandle _memory, WdSize _size, WdBufferUsageFlags _usage) :
                handle(_handle),
                memory(_memory),
                usage(_usage),
                size(_size) {};
            
            void* data = nullptr; // nullptr by default 
    };
}
#endif
