#ifndef WADO_GRAPHICS_ABSTRACTION_LAYER
#define WADO_GRAPHICS_ABSTRACTION_LAYER

#include "Shader.h"

#include <cstdint>
#include <vector>
#include <memory>

class Wado::Shader::Shader;

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
        WD_FORMAT_R32G32B32_SFLOAT,
        WD_FORMAT_R32G32_SFLOAT,
        WD_FORMAT_R32_SINT,
        WD_FORMAT_D32_SFLOAT,
        WD_FORMAT_D32_SFLOAT_S8_UINT,
        WD_FORMAT_D24_UNORM_S8_UINT,
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
        WD_UNDEFINED = 0x0,
        WD_TRANSFER_SRC = 0x00000001,
        WD_TRANSFER_DST = 0x00000002,
        WD_SAMPLED_IMAGE = 0x00000004,
        WD_STORAGE_IMAGE = 0x00000008,
        WD_COLOR_ATTACHMENT = 0x00000010,
        WD_DEPTH_STENCIL_ATTACHMENT = 0x00000020,
        WD_INPUT_ATTACHMENT = 0x00000040,
        WD_TRANSIENT_ATTACHMENT = 0x00000080,
    };

    enum WdImageTiling {
        WD_TILING_OPTIMAL,
        WD_TILING_LINEAR
    }

    using WdFormatFeatureFlags = uint32_t;

    // basically 1-to-1 with vulkan 
    enum WdFormatFeatures {
        WD_FORMAT_FEATURE_SAMPLED_IMAGE = 0x00000001,
        WD_FORMAT_FEATURE_STORAGE_IMAGE = 0x00000002,
        WD_FORMAT_FEATURE_STORAGE_IMAGE_ATOMIC = 0x00000004,
        WD_FORMAT_FEATURE_UNIFORM_TEXEL_BUFFER = 0x00000008,
        WD_FORMAT_FEATURE_STORAGE_TEXEL_BUFFER = 0x00000010,
        WD_FORMAT_FEATURE_STORAGE_TEXEL_BUFFER_ATOMIC = 0x00000020,
        WD_FORMAT_FEATURE_VERTEX_BUFFER = 0x00000040,
        WD_FORMAT_FEATURE_COLOR_ATTACHMENT = 0x00000080,
        WD_FORMAT_FEATURE_COLOR_ATTACHMENT_BLEND = 0x00000100,
        WD_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT = 0x00000200,
        WD_FORMAT_FEATURE_BLIT_SRC = 0x00000400,
        WD_FORMAT_FEATURE_BLIT_DST = 0x00000800,
        WD_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR = 0x00001000,
    }

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

    using WdTexture = struct WdTexture {
        std::string name;
        uint32_t uid;   
        WdImage image;
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

    enum WdInputTopology {
        WD_TOPOLOGY_TRIANGLE_LIST,
        WD_TOPOLOGY_POINT_LIST,
        WD_TOPOLOGY_LINE_LIST,
    };

    class WdVertexBuilder {
        public:
            virtual std::vector<WdVertexBinding> getBindingDescriptions() = 0;
            virtual WdInputTopology getInputTopology() = 0;
    };

    using WdViewportProperties = struct WdViewportProperties {
        WdExtent2D startCoords;
        WdExtent2D endCoords;
        struct depth {
            float min = 0.0f;
            float max = 1.0f;
        };
        struct scissor {
            WdExtent2D offset;
            WdExtent2D extent;
        };
    };

    class WdPipeline {
        public:
        private:
            WdPipeline(Shader::Shader vertexShader, Shader::Shader fragmentShader, WdVertexBuilder vertexBuilder, WdViewportProperties viewportProperties);
            Shader::Shader _vertexShader;
            Shader::Shader _fragmentShader;
    };

    class WdRenderPass {
        public:
        private:
            WdRenderPass(std::vector<WdPipeline> pipelines);
            std::vector<WdPipeline> _pipelines;
    };

    class WdCommandList {
        public:
            friend class GraphicsLayer;

            virtual void resetCommandList() = 0;
            virtual void beginCommandList() = 0;
            virtual void setRenderPass(WdRenderPass renderPass) = 0;
            virtual void nextPipeline() = 0;
            virtual void setVertexBuffer(WdBuffer vertexBuffer) = 0;
            virtual void setIndexBuffer(WdBuffer indexBuffer) = 0;
            virtual void setViewport(WdViewportProperties WdViewportProperties) = 0;
            virtual void drawIndexed() = 0;
            virtual void drawVertices(uint32_t vertexCount) = 0;
            virtual void endRenderPass() = 0;
            virtual void endCommandList() = 0;
            virtual void execute(WdFenceHandle fenceToSignal) = 0;
        private:
            WdCommandList();
    };

    class GraphicsLayer {
        public:
            virtual void init() = 0;

            // no sharing mode yet, will infer from usage I think.
            // for tiling, default optimal?
            // figure out memory req from usage 
            virtual WdImage create2DImage(WdExtent2D extent, uint32_t mipLevels, 
                    WdSampleCount sampleCount, WdFormat imageFormat, WdImageUsageFlags usageFlags) = 0;

            virtual WdBufferHandle createBuffer(WdSize size, WdBufferUsageFlags usageFlags) = 0;

            virtual void updateBuffer(WdBuffer buffer, void * data, WdSize offset, WdSize dataSize) = 0;

            virtual void copyBufferToImage(WdBufferHandle buffer, WdImageHandle image, WdExtent2D extent) = 0;

            virtual void copyBuffer(WdBufferHandle srcBuffer, WdBufferHandle dstBuffer, WdSize size) = 0;

            virtual WdFenceHandle createFence(bool signaled = true) = 0;

            virtual WdSemaphoreHandle createSemaphore() = 0;

            virtual void waitForFences(std::vector<WdFenceHandle> fences, bool waitAll = true, uint64_t timeout = UINT64_MAX) = 0;

            virtual void resetFences(std::vector<WdFenceHandle> fences) = 0;

            virtual WdPipeline createPipeline(Shader::Shader vertexShader, Shader::Shader fragmentShader, WdVertexBuilder* vertexBuilder, WdViewportProperties viewportProperties) = 0;

            virtual WdRenderPass createRenderPass(std::vector<WdPipeline> pipelines, std::vector<WdImage> attachments) = 0;

            virtual std::unique_ptr<WdCommandList> createCommandList() = 0;

            virtual WdFormat findSupportedHardwareFormat(const std::vector<WdFormat>& formatCandidates, WdImageTiling tiling, WdFormatFeatureFlags features) = 0;
            // this i can definitely automate with a render graph, a lot of this actually. 
            virtual void prepareImageFor(WdImage image, WdImageUsage currentUsage, WdImageUsage nextUsage) = 0;

            virtual void presentCurrentFrame() = 0;
    };
}

#endif