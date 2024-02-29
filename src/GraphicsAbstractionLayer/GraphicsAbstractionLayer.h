#ifndef WADO_GRAPHICS_ABSTRACTION_LAYER
#define WADO_GRAPHICS_ABSTRACTION_LAYER

#include <cstdint>
#include <vector>

namespace Wado::GAL {

    using WdImageHandle = void *; // not sure about this here?
    using WdBufferHandle = void *;
    using WdRenderTarget = void *; // this would be the image view 
    using WdMemoryPointer = void *;

    using WdFenceHandle = void *; 
    using WdSemaphoreHandle = void *;


    using WdSize = size_t;

    using WdExtent2D = struct WdExtent2D {
        uint32_t height;
        uint32_t width;
    };

    using WdExtent3D = struct WdExtent3D {
        uint32_t height;
        uint32_t width;
        uint32_t depth;
    };

    enum WdFormat {
        WD_FORMAT_R8G8B8A8_UINT,
        WD_FORMAT_R8G8B8A8_SINT,
        WD_FORMAT_R8G8B8A8_SRGB,
        WD_FORMAT_UNDEFINED,
        WD_FORMAT_R32G32B32A32_UINT,
        WD_FORMAT_R32G32B32A32_SINT,
        WD_FORMAT_R32G32B32A32_SFLOAT,
    };

    using WdBufferUsageFlags = uint32_t;

    enum WdBufferUsage {
        WD_TRANSFER_SRC,
        WD_TRANSFER_DST,
        WD_UNIFORM_BUFFER,
        WD_STORAGE_BUFFER,
        WD_INDEX_BUFFER,
        WD_VERTEX_BUFFER,
        WD_UNIFORM_TEXEL_BUFFER,
        WD_STORAGE_TEXEL_BUFFER,
        WD_INDIRECT_BUFFER,
    };


    using WdImageUsageFlags = uint32_t;
    
    enum WdImageUsage {
        WD_TRANSFER_SRC,
        WD_TRANSFER_DST,
        WD_SAMPLED_IMAGE,
        WD_STORAGE_IMAGE,
        WD_COLOR_ATTACHMENT,
        WD_DEPTH_STENCIL_ATTACHMENT,
        WD_INPUT_ATTACHMENT,
        WD_TRANSIENT_ATTACHMENT,
    };

    enum WdSampleCount {
        WD_SAMPLE_COUNT_1,
        WD_SAMPLE_COUNT_2, 
        WD_SAMPLE_COUNT_4, 
        WD_SAMPLE_COUNT_8, 
        WD_SAMPLE_COUNT_16,
        WD_SAMPLE_COUNT_32,
        WD_SAMPLE_COUNT_64,
    };

    using WdImage = struct WdImage {
        WdImageHandle handle;
        WdMemoryPointer memory;
        WdRenderTarget target;
        WdFormat format;
        WdExtent3D extent;
    };

    using WdTexture = struct WdTexture : public WdImage {

    };

    using WdBuffer = struct WdBuffer {
        WdBufferHandle handle;
        WdMemoryPointer memory;
        WdSize size;
    };

    enum WdVertexRate {
        WD_VERTEX_RATE_VERTEX,
        WD_VERTEX_RATE_INSTANCE,
    };

    using WdVertexAttribute = struct WdVertexAttribute {
        WdFormat format;
        WdSize offset;
    };

    using WdVertexBinding = struct WdVertexBinding {
        WdSize stride;
        WdVertexRate rate;
        std::vector<WdVertexAttribute> attributeDescriptions;
    };

    class WdVertexBuilder {
        public:
            virtual std::vector<WdVertexBinding> getBindingDescriptions() = 0;
    };

    class GraphicsLayer {
        public:
            virtual void init() = 0;

            virtual WdImageHandle create2DImage(WdExtent2D extent, uint32_t mipLevels, 
                    WdSampleCount sampleCount, WdFormat imageFormat, WdImageUsageFlags usageFlags) = 0;

            virtual WdBufferHandle createBuffer(WdSize size, WdBufferUsageFlags usageFlags) = 0;

            virtual void copyBufferToImage(WdBufferHandle buffer, WdImageHandle image, WdExtent2D extent) = 0;

            virtual void copyBuffer(WdBufferHandle srcBuffer, WdBufferHandle dstBuffer, WdSize size) = 0;

            virtual WdFenceHandle createFence(bool signaled = true) = 0;

            virtual WdSemaphoreHandle createSemaphore() = 0;

            virtual void waitForFences(std::vector<WdFenceHandle> fences, bool waitAll = true, uint64_t timeout = UINT64_MAX) = 0;

            virtual void resetFences(std::vector<WdFenceHandle> fences) = 0;
    };
}

#endif