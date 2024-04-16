#ifndef WADO_GRAPHICS_ABSTRACTION_LAYER
#define WADO_GRAPHICS_ABSTRACTION_LAYER

#include "Shader.h"
#include "VulkanLayer.h"

#include <cstdint>
#include <vector>
#include <tuple>
#include <memory>
#include <map>
#include <string>

class Wado::Shader::Shader;

namespace Wado::GAL {

    using WdImageHandle = void *; // not sure about this yet
    using WdBufferHandle = void *;
    using WdSamplerHandle = void *;
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

    // matches Vulkan
    enum WdFormat {
        WD_FORMAT_UNDEFINED,
        WD_FORMAT_R8_UNORM,
        WD_FORMAT_R8_SINT,
        WD_FORMAT_R8_UINT,
        WD_FORMAT_R8G8_UNORM,
        WD_FORMAT_R8G8_SINT,
        WD_FORMAT_R8G8_UINT,
        WD_FORMAT_R8G8B8_UNORM,
        WD_FORMAT_R8G8B8_SINT,
        WD_FORMAT_R8G8B8_UINT,
        WD_FORMAT_R8G8B8A8_UNORM,
        WD_FORMAT_R8G8B8A8_SINT,
        WD_FORMAT_R8G8B8A8_UINT,
        WD_FORMAT_R16_SINT,
        WD_FORMAT_R16_UINT,
        WD_FORMAT_R16_SFLOAT,
        WD_FORMAT_R16G16_SINT,
        WD_FORMAT_R16G16_UINT,
        WD_FORMAT_R16G16_SFLOAT,
        WD_FORMAT_R16G16B16_SINT,
        WD_FORMAT_R16G16B16_UINT,
        WD_FORMAT_R16G16B16_SFLOAT,
        WD_FORMAT_R16G16B16A16_SINT,
        WD_FORMAT_R16G16B16A16_UINT,
        WD_FORMAT_R16G16B16A16_SFLOAT,
        WD_FORMAT_R32_SINT,
        WD_FORMAT_R32_UINT,
        WD_FORMAT_R32_SFLOAT,
        WD_FORMAT_R32G32_SINT,
        WD_FORMAT_R32G32_UINT,
        WD_FORMAT_R32G32_SFLOAT,
        WD_FORMAT_R32G32B32_SINT,
        WD_FORMAT_R32G32B32_UINT,
        WD_FORMAT_R32G32B32_SFLOAT,
        WD_FORMAT_R32G32B32A32_SINT,
        WD_FORMAT_R32G32B32A32_UINT,
        WD_FORMAT_R32G32B32A32_SFLOAT,
        WD_FORMAT_R64_SINT,
        WD_FORMAT_R64_UINT,
        WD_FORMAT_R64_SFLOAT,
        WD_FORMAT_R64G64_SINT,
        WD_FORMAT_R64G64_UINT,
        WD_FORMAT_R64G64_SFLOAT,
        WD_FORMAT_R64G64B64_SINT,
        WD_FORMAT_R64G64B64_UINT,
        WD_FORMAT_R64G64B64_SFLOAT,
        WD_FORMAT_R64G64B64A64_SINT,
        WD_FORMAT_R64G64B64A64_UINT,
        WD_FORMAT_R64G64B64A64_SFLOAT,
    };

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

    using WdImageUsageFlags = uint32_t;
    
    enum WdImageUsage {
        WD_UNDEFINED = 0x0,
        WD_TRANSFER_SRC = 0x00000001,
        WD_TRANSFER_DST = 0x00000002,
        WD_SAMPLED_IMAGE = 0x00000004,
        WD_STORAGE_IMAGE = 0x00000008,
        WD_COLOR_ATTACHMENT = 0x00000010,
        WD_DEPTH_STENCIL_ATTACHMENT = 0x00000020,
        WD_TRANSIENT_ATTACHMENT = 0x00000040,
        WD_INPUT_ATTACHMENT = 0x00000080,
        WD_PRESENT = 0x10000000, // special, GAL-only. This is used later on in the Vulkan backend for image layouts 
    };

    enum WdImageTiling {
        WD_TILING_OPTIMAL,
        WD_TILING_LINEAR
    };

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
    };

    enum WdSampleCount {
        WD_SAMPLE_COUNT_1 = 0x00000001,
        WD_SAMPLE_COUNT_2 = 0x00000002, 
        WD_SAMPLE_COUNT_4 = 0x00000004, 
        WD_SAMPLE_COUNT_8 = 0x00000008, 
        WD_SAMPLE_COUNT_16 = 0x00000010,
        WD_SAMPLE_COUNT_32 = 0x00000020,
        WD_SAMPLE_COUNT_64 = 0x00000040,
    };

    // same as Vulkan
    enum WdFilterMode {
        WD_NEAREST_NEIGHBOUR = 0,
        WD_LINEAR = 1,
    };

    // same as Vulkan
    enum WdAddressMode {
        WD_REPEAT = 0,
        WD_MIRROR_REPEAT = 1,
        WD_CLAMP_TO_EDGE = 2, 
        WD_CLAMP_TO_BORDER = 3,
    };

    using WdTextureAddressMode = struct WdTextureAddressMode {
        WdAddressMode uMode;
        WdAddressMode vMode;
        WdAddressMode wMode; 
    };

    const WdTextureAddressMode DefaultTextureAddressMode = {WdAddressMode::WD_REPEAT, WdAddressMode::WD_REPEAT, WdAddressMode::WD_REPEAT};

    using WdColorValue = struct WdColorValue {
        float r;
        float g;
        float b;
        float a;
    };

    using WdDepthStencilValue = struct WdDepthStencilValue {
        float depth;
        int stencil;
    };

    using WdClearValue = union WdClearValue {
        WdColorValue color;
        WdDepthStencilValue depthStencil;
    };

    const WdDepthStencilValue defaultDepthStencilClear = {1.0f, 0};
    const WdColorValue defaultColorClear = {0.0f, 0.0f, 0.0f, 1.0f};

    // TODO: these should also have private constructors.
    using WdImage = struct WdImage {
            friend class GraphicsLayer;
            friend class Vulkan::VulkanLayer;

            const WdImageHandle handle;
            const WdMemoryPointer memory;
            const WdRenderTarget target;
            const WdFormat format;
            const WdExtent3D extent;
            const WdImageUsageFlags usage;
            const WdClearValue clearValue;
        private:
            WdImage(WdImageHandle _handle, WdMemoryPointer _memory, WdRenderTarget _target, WdFormat _format, WdExtent3D _extent, WdImageUsageFlags _usage, WdClearValue _clearValue) : 
                handle(_handle), 
                memory(_memory),
                target(_target),
                format(_format),
                extent(_extent),
                usage(_usage),
                clearValue(_clearValue) {};
    };

    using WdTexture = struct WdTexture {
        std::string name;
        uint32_t uid; 
        WdImage image;
    };

    using WdBuffer = struct WdBuffer {
            friend class GraphicsLayer;
            friend class Vulkan::VulkanLayer;

            const WdBufferHandle handle;
            const WdMemoryPointer memory;
            const WdSize size;
        private:
            WdBuffer(WdBufferHandle _handle, WdMemoryPointer _memory, WdSize _size) :
                handle(_handle),
                memory(_memory),
                size(_size) {};
            void* data;
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

    enum WdStage = {
        Unknown = 0x00000000,
        Vertex = 0x00000001,
        Fragment = 0x00000002,
    };

    using WdStageMask = uint32_t;

    class WdPipeline {
        public:
            friend class GraphicsLayer;
            friend class Vulkan::VulkanLayer;

            friend class WdRenderPass;
            friend class Vulkan::VulkanRenderPass;

            using WdImageResource = struct WdImageResource {
                WdImage* image;
                WdSamplerHandle sampler;
            };

            using WdBufferResource = struct WdBufferResource {
                WdBuffer* buffer;
                WdRenderTarget bufferTarget;
            };

            union ShaderResource {
                WdImageResource imageResource;
                WdBufferResource bufferResource;

                ShaderResource(WdImage* img) {
                    // Should have default values for sampler
                    imageResource.image = img;
                };

                ShaderResource(WdBuffer* buf) {
                    // should have default values for Buffer View
                    bufferResource.buffer = buf;
                };

                ShaderResource(WdSamplerHandle sampler) {
                    // should have default value for img 
                    imageResource.sampler = sampler;
                };

                ShaderResource(WdImage* img, WdSamplerHandle sampler) {
                    imageResource.image = img;
                    imageResource.sampler = sampler;
                };
            };

            static const int UNIFORM_END = -1;

            // subpass inputs handled in setUniform 
            void setUniform(const WdStage stage, const std::string& paramName, ShaderResource resource);

            // for array params
            void setUniform(const WdStage stage, const std::string& paramName, std::vector<ShaderResource>& resources);

            void addToUniform(const WdStage stage, const std::string& paramName, ShaderResource resource, int index = UNIFORM_END);

            // special case, since it relates to the framebuffer
            void setFragmentOutput(const std::string& paramName, WdImageResource resource);

            void setDepthStencilResource(WdImageResource resource);

        private:
        // Util types 
            enum ShaderParameterType {
                WD_SAMPLED_IMAGE = 0, // sampler2D
                WD_TEXTURE_IMAGE = 1, // just texture2D
                WD_STORAGE_IMAGE = 2, // read-write
                WD_SAMPLED_BUFFER = 3,
                WD_BUFFER_IMAGE = 4,
                WD_UNIFORM_BUFFER = 5,
                WD_SUBPASS_INPUT = 6, // only supported by Vulkan
                WD_STORAGE_BUFFER = 7, 
                WD_STAGE_OUTPUT = 8, // used for outs 
                WD_SAMPLER = 9,
                WD_PUSH_CONSTANT, // only supported by Vulkan backend
                WD_STAGE_INPUT, // used for ins 
            };

            using VertexInput = struct VertexInput {
                ShaderParameterType paramType;
                uint8_t decorationLocation;
                uint8_t decorationBinding; // I think this should be unused for now 
                uint8_t offset;
                uint8_t size;
                WdFormat format;
            };

            using Uniform = struct Uniform {
                ShaderParameterType paramType;
                uint8_t decorationSet;
                uint8_t decorationBinding;
                uint8_t decorationLocation;
                uint8_t resourceCount;
                WdStageMask stages;
                std::vector<ShaderResource> resources;
            };

            using SubpassInput = struct SubpassInput { // Vulkan only 
                ShaderParameterType paramType; // always subpass input, doesn't really matter 
                uint8_t decorationSet;
                uint8_t decorationBinding;
                uint8_t decorationLocation;
                uint8_t decorationIndex;
                WdImageResource resource; // should always be img resource
            }; 

            using FragmentOutput = struct FragmentOutput { 
                ShaderParameterType paramType; // always stage output, doesn't really matter 
                uint8_t decorationIndex; // needed for refs 
                WdImageResource resource; // should always be img resource
            }; 

            using UniformAddress = std::tuple<uint8_t, uint8_t, uint8_t>; // (Set, Binding, Location)
            using UniformIdent = std::tuple<std::string, WdStage>; // (uniform name, stage its in)

            using VertexInputs = std::vector<VertexInput>;
            using Uniforms = std::map<UniformAddress, Uniform>;
            using UniformAddresses = std::map<UniformIdent, UniformAddress>;
            using SubpassInputs = std::map<std::string, SubpassInput>;
            using FragmentOutputs = std::map<std::string, FragmentOutput>;

            WdPipeline(Shader::ShaderByteCode vertexShader, Shader::ShaderByteCode fragmentShader, WdViewportProperties viewportProperties);

            void addUniformDescription(spirv_cross::Compiler& spirvCompiler, const std::vector<spirv_cross::Resource>& resources, const ShaderParameterType paramType, const WdStage stage);

            void generateVertexParams();
            void generateFragmentParams();
            
            Shader::ShaderByteCode _vertexShader;
            Shader::ShaderByteCode _fragmentShader;

            UniformAddresses _uniformAddresses;
            Uniforms _uniforms;

            VertexInputs _vertexInputs;
            SubpassInputs _subpassInputs; // fragment & Vulkan only
            FragmentOutputs _fragmentOutputs;

            WdImageResource _depthStencilResource;

            WdViewportProperties _viewportProperties
    };

    class WdRenderPass {
        public:
            friend class GraphicsLayer;
            friend class Vulkan::VulkanLayer;

        private:
            WdRenderPass(std::vector<WdPipeline> pipelines);
            virtual void init() = 0;
            std::vector<WdPipeline> _pipelines;
    };

    class WdCommandList {
        public:
            friend class GraphicsLayer;
            friend class Vulkan::VulkanLayer;

            virtual void resetCommandList() = 0;
            virtual void beginCommandList() = 0;
            virtual void setRenderPass(WdRenderPass renderPass) = 0;
            virtual void nextPipeline() = 0;
            virtual void setVertexBuffers(std::vector<WdBuffer> vertexBuffer) = 0;
            virtual void setIndexBuffer(WdBuffer indexBuffer) = 0;
            virtual void setViewport(WdViewportProperties WdViewportProperties) = 0;
            virtual void drawIndexed() = 0;
            virtual void drawVertices(uint32_t vertexCount) = 0;
            virtual void endRenderPass() = 0;
            virtual void endCommandList() = 0;
            virtual void execute(WdFenceHandle fenceToSignal) = 0;
            // non-immediate versions 
            virtual void copyBufferToImage(WdBuffer& buffer, WdImage& image, WdExtent2D extent) = 0;
            virtual void copyBuffer(WdBuffer& srcBuffer, WdBuffer& dstBuffer, WdSize size) = 0;
        private:
            WdCommandList();
    };

    class GraphicsLayer {
        public:
            virtual void init() = 0;

            virtual WdImage& create2DImage(WdExtent2D extent, uint32_t mipLevels, 
                    WdSampleCount sampleCount, WdFormat imageFormat, WdImageUsageFlags usageFlags) = 0;

            virtual WdBuffer& createBuffer(WdSize size, WdBufferUsageFlags usageFlags) = 0;

            // create texture sampler 
            virtual WdSamplerHandle createSampler(WdTextureAddressMode addressMode = DefaultTextureAddressMode, WdFilterMode minFilter = WdFilterMode::WD_LINEAR, WdFilterMode magFilter = WdFilterMode::WD_LINEAR, WdFilterMode mipMapFilter = WdFilterMode::WD_LINEAR) = 0;

            virtual void updateBuffer(const WdBuffer& buffer, void *data, WdSize offset, WdSize dataSize) = 0;
            // close or open "pipe" between CPU and GPU for this buffer's memory
            virtual void openBuffer(WdBuffer& buffer) = 0;

            virtual void closeBuffer(WdBuffer& buffer) = 0;

            // immediate versions 
            virtual void copyBufferToImage(WdBuffer& buffer, WdImage& image, WdExtent2D extent) = 0;
            virtual void copyBuffer(WdBuffer& srcBuffer, WdBuffer& dstBuffer, WdSize size) = 0;

            virtual WdFenceHandle createFence(bool signaled = true) = 0;

            //virtual WdSemaphoreHandle createSemaphore() = 0;

            virtual void waitForFences(const std::vector<WdFenceHandle>& fences, bool waitAll = true, uint64_t timeout = UINT64_MAX) = 0;

            virtual void resetFences(const std::vector<WdFenceHandle>& fences) = 0;

            virtual WdPipeline createPipeline(Shader::Shader vertexShader, Shader::Shader fragmentShader, WdViewportProperties viewportProperties) = 0;

            virtual WdRenderPass createRenderPass(const std::vector<WdPipeline>& pipelines) = 0;

            virtual std::unique_ptr<WdCommandList> createCommandList() = 0;

            virtual WdFormat findSupportedHardwareFormat(const std::vector<WdFormat>& formatCandidates, WdImageTiling tiling, WdFormatFeatureFlags features) = 0;
            // this i can definitely automate with a render graph, a lot of this actually.
            // this is also immediate
            virtual void prepareImageFor(const WdImage& image, WdImageUsage currentUsage, WdImageUsage nextUsage) = 0;

            virtual void presentCurrentFrame() = 0;

            virtual void createRenderingSurfaces() = 0;
    };
}

#endif