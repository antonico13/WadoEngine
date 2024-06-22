#ifndef WADO_GRAPHICS_ABSTRACTION_LAYER
#define WADO_GRAPHICS_ABSTRACTION_LAYER

#include "WdCommon.h"
#include "WdImage.h"
#include "WdBuffer.h"
#include "WdPipeline.h"
#include "WdCommandList.h"

#include <vector>

namespace Wado::Shader {

    using WdShaderByteCode = const std::vector<uint8_t>&;

    class WdShader {
        public:
            WdShaderByteCode shaderByteCode;
        private:
    };
};

namespace Wado::GAL {

    using WdPipelineCache = WdHandle;

    enum WdPipelineCreateFlag {
        WD_PIPELINE_ALLOW_DERIVATIVES = 0x00000002,
        WD_PIPELINE_DERIVATIE = 0x00000004,
    };
    
    // 1 to 1 with Vulkan
    enum WdCompareOp {
        WD_COMPARE_OP_NEVER = 0,
        WD_COMPARE_OP_LESS = 1,
        WD_COMPARE_OP_EQUAL = 2,
        WD_COMPARE_OP_LESS_OR_EQUAL = 3,
        WD_COMPARE_OP_GREATER = 4,
        WD_COMPARE_OP_NOT_EQUAL = 5,
        WD_COMPARE_OP_GREATER_OR_EQUAL = 6,
        WD_COMPARE_OP_ALWAYS = 7,
    };

    // TODO: unused for now 
    using WdMultisamplingInfo = struct WdMultisamplingInfo {

    };

    // 1-to-1 with Vulkan
    enum WdStencilOp {
        WD_STENCIL_OP_KEEP = 0,
        WD_STENCIL_OP_ZERO = 1,
        WD_STENCIL_OP_REPLACE = 2,
        WD_STENCIL_OP_INCREMENT_AND_CLAMP = 3,
        WD_STENCIL_OP_DECREMENT_AND_CLAMP = 4,
        WD_STENCIL_OP_INVERT = 5,
        WD_STENCIL_OP_INCREMENT_AND_WRAP = 6,
        WD_STENCIL_OP_DECREMENT_AND_WRAP = 7,
    };

    // Similar to Vulkan
    using WdStencilTestInfo = struct WdStencilTestInfo {
        WdStencilOp failureOp; // what to do with samples that fail the stencil test
        WdStencilOp successOp; // what to do with samples that pass both depth and stencil test 
        WdStencilOp depthFailOp; // what to do with samples that pass stencil but fail depth test
        WdCompareOp compareOp; // what operator to perform stencil test with 
        uint32_t compareMask; // bit-mask of which bits to use in the test
        uint32_t writeMask; // bit-mask of which bits to update in the resource after the test 
        uint32_t reference; // value to use as a reference in the tests 
    };

    // Similar to Vulkan
    // TODO: could I break this into multiple structs?
    using WdDepthStencilStateInfo = struct WdDepthStencilStateInfo {
        bool depthTestEnable;
        bool depthWriteEnable; // whether to update the depth information when the depth test is enabled
        WdCompareOp depthCompareOp; // how to perform the depth test
        bool depthBoundsTestEnable; // whether to perform bounds testing in the depth test 
        WdDepthBounds depthBounds; // bounds used for depth testing 

        bool stencilTestEnable; // whether to perform stencil test
        WdStencilTestInfo front; // test details for front-facing stencil tests
        WdStencilTestInfo back; // test details for back-facing stencil tests 
    };

    // 1-to-1 with Vulkan
    enum WdBlendFactor {
        WD_BLEND_FACTOR_ZERO = 0,
        WD_BLEND_FACTOR_ONE = 1,
        WD_BLEND_FACTOR_SRC_COLOR = 2,
        WD_BLEND_FACTOR_ONE_MINUS_SRC_COLOR = 3,
        WD_BLEND_FACTOR_DST_COLOR = 4,
        WD_BLEND_FACTOR_ONE_MINUS_DST_COLOR = 5,
        WD_BLEND_FACTOR_SRC_ALPHA = 6,
        WD_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA = 7,
        WD_BLEND_FACTOR_DST_ALPHA = 8,
        WD_BLEND_FACTOR_ONE_MINUS_DST_ALPHA = 9,
        WD_BLEND_FACTOR_CONSTANT_COLOR = 10,
        WD_BLEND_FACTOR_ONE_MINUS_CONSTANT_COLOR = 11,
        WD_BLEND_FACTOR_CONSTANT_ALPHA = 12,
        WD_BLEND_FACTOR_ONE_MINUS_CONSTANT_ALPHA = 13,
        WD_BLEND_FACTOR_SRC_ALPHA_SATURATE = 14,
        WD_BLEND_FACTOR_SRC1_COLOR = 15,
        WD_BLEND_FACTOR_ONE_MINUS_SRC1_COLOR = 16,
        WD_BLEND_FACTOR_SRC1_ALPHA = 17,
        WD_BLEND_FACTOR_ONE_MINUS_SRC1_ALPHA = 18,
    };

    // 1-to-1 with Vulkan 
    enum WdBlendOp {
        WD_BLEND_OP_ADD = 0,
        WD_BLEND_OP_SUBTRACT = 1,
        WD_BLEND_OP_REVERSE_SUBTRACT = 2,
        WD_BLEND_OP_MIN = 3,
        WD_BLEND_OP_MAX = 4,
    };

    enum WdColorComponents {
        WD_COLOR_COMPONENT_R = 0x00000001,
        WD_COLOR_COMPONENT_G = 0x00000002,
        WD_COLOR_COMPONENT_B = 0x00000004,
        WD_COLOR_COMPONENT_A = 0x00000008,
    };

    using WdColorComponentsFlags = uint64_t;

    using WdRenderTargetColorBlend = struct WdRenderTargetColorBlend {
        bool blendEnable;
        WdBlendFactor srcColorBlendFactor;
        WdBlendFactor dstColorBlendFactor;
        WdBlendOp colorBlendOp;
        WdBlendFactor srcAlphaBlendFactor;
        WdBlendFactor dstAlphaBlendFactor;
        WdBlendOp alphaBlendOp;
        WdColorComponentsFlags colorMask; // which color components to blend 
    };

    using WdRenderTargetInfo = struct WdRenderTargetInfo {
        WdRenderTargetColorBlend colorBlend;
        // TODO add missing stuff here 
    };

    // 1-to-1 with Vulkan
    enum WdLogicOp {
        WD_LOGIC_OP_CLEAR = 0,
        WD_LOGIC_OP_AND = 1,
        WD_LOGIC_OP_AND_REVERSE = 2,
        WD_LOGIC_OP_COPY = 3,
        WD_LOGIC_OP_AND_INVERTED = 4,
        WD_LOGIC_OP_NO_OP = 5,
        WD_LOGIC_OP_XOR = 6,
        WD_LOGIC_OP_OR = 7,
        WD_LOGIC_OP_NOR = 8,
        WD_LOGIC_OP_EQUIVALENT = 9,
        WD_LOGIC_OP_INVERT = 10,
        WD_LOGIC_OP_OR_REVERSE = 11,
        WD_LOGIC_OP_COPY_INVERTED = 12,
        WD_LOGIC_OP_OR_INVERTED = 13,
        WD_LOGIC_OP_NAND = 14,
        WD_LOGIC_OP_SET = 15,
    };

    using WdBlendStateInfo = struct WdBlendStateInfo {
        bool logicOpEnable; // if enable, blending is done with a logical operator, otherwise color blending is performed 
        WdLogicOp logicOp;
        std::vector<WdRenderTargetInfo> renderTargetInfos;
        WdBlendConstant blendConstant;
    };

    using WdComputePipelineCreateInfo = struct WdComputePipelineCreateInfo {

    };

    using WdGraphicsPipelineCreateInfo = struct WdGraphicsPipelineCreateInfo {
        // These should be clone pointers since they all need to survive 
        const Shader::WdShader *vertexShader;
        const Shader::WdShader *fragmentShader;
        const WdViewport *viewport;
        const WdMultisamplingInfo *multisamplingInfo;
        const WdDepthStencilStateInfo *depthStencilInfo;
        const WdBlendStateInfo *blendStateInfo;
        // TODO: pipeline dynamic state here?? How should users choose?
        // Vertex, input assembly, and rasterizer -> handled automatically?

        // TODO: how to do base pipeline here?
    };

    using WdComputePipelineHandle = WdHandle;
    using WdGraphicsPipelineHandle = WdHandle; 
    using WdRenderPassHandle = WdHandle;

    // Graphics abstraction layer.
    // Backends such as Vulkan or Direct X implement this interface,
    // and all rendering code should be written using these functions
    class WdLayer {
        public:
            // Utils as static consts here for now 
            static const uint32_t FRAMES_IN_FLIGHT = 3;
            static const int CURRENT_FRAME_RESOURCE = -1;

            // Returns resource handle for the current display target
            virtual WdImageResourceHandle getDisplayTarget() = 0;
            
            virtual WdImageHandle create2DImage(WdExtent2D extent, WdFormat imageFormat, WdMipCount mipLevels, 
                    WdSampleCount sampleCount, WdImageUsageFlags usageFlags, bool multiFrame = false) = 0;
            // TODO: vkComponentMapping here?
            virtual WdImageResourceHandle create2DImageShaderResource(WdImageHandle image, WdFormat resourceFormat, WdImageSubpartsInfo imageSubpartInfo) = 0;

            virtual WdBufferHandle createBuffer(WdSize size, WdBufferUsageFlags usageFlags, bool multiFrame = false) = 0;
            // If using the buffer a shader resource 
            // TODO: add default values for offset and range here 
            virtual WdBufferResourceHandle createBufferShaderResource(WdBufferHandle buffer, WdFormat resourceFormat, WdSize offset, WdSize range) = 0;
            // TODO: optionally externally synchronised?
            virtual WdPipelineCache createPipelineCache(const void* initialPipelineCache = nullptr, WdSize initialPipelineCacheSize = 0) = 0;

            virtual std::vector<WdComputePipelineHandle> createComputePipelines(const std::vector<WdComputePipelineCreateInfo>& pipelineInfos) = 0;
            virtual std::vector<WdGraphicsPipelineHandle> createGraphicsPipelines(const std::vector<WdGraphicsPipelineCreateInfo>& pipelineInfos) = 0;
            virtual WdRenderPassHandle createRenderPass(const std::vector<WdGraphicsPipelineHandle>& pipelines) = 0;

            // create texture sampler 
            virtual WdSamplerHandle createSampler(const WdTextureAddressMode& addressMode = DEFAULT_TEXTURE_ADDRESS_MODE, WdFilterMode minFilter = WdFilterMode::WD_LINEAR, WdFilterMode magFilter = WdFilterMode::WD_LINEAR, WdFilterMode mipMapFilter = WdFilterMode::WD_LINEAR) = 0;

            virtual void updateBuffer(const WdBuffer& buffer, void *data, WdSize offset, WdSize dataSize, int bufferIndex = CURRENT_FRAME_RESOURCE) = 0;
            // close or open "pipe" between CPU and GPU for this buffer's memory
            virtual void openBuffer(WdBuffer& buffer, int bufferIndex = CURRENT_FRAME_RESOURCE) = 0;

            virtual void closeBuffer(WdBuffer& buffer, int bufferIndex = CURRENT_FRAME_RESOURCE) = 0;

            // immediate versions 
            virtual void copyBufferToImage(const WdBuffer& buffer, const WdImage& image, WdExtent2D extent, int resourceIndex = CURRENT_FRAME_RESOURCE) = 0;
            virtual void copyBuffer(const WdBuffer& srcBuffer, const WdBuffer& dstBuffer, WdSize size, int bufferIndex = CURRENT_FRAME_RESOURCE) = 0;

            virtual WdFenceHandle createFence(bool signaled = true) = 0;

            virtual WdSemaphoreHandle createSemaphore() = 0;

            virtual void waitForFences(const std::vector<WdFenceHandle>& fences, bool waitAll = true, uint64_t timeout = UINT64_MAX) = 0;

            virtual void resetFences(const std::vector<WdFenceHandle>& fences) = 0;

            virtual Memory::WdClonePtr<WdPipeline> createPipeline(const Shader::WdShader& vertexShader, const Shader::WdShader& fragmentShader, const WdViewportProperties& viewportProperties) = 0;

            virtual Memory::WdClonePtr<WdRenderPass> createRenderPass(const std::vector<WdPipeline>& pipelines) = 0;

            virtual Memory::WdClonePtr<WdCommandList> createCommandList() = 0;

            virtual void executeCommandList(const WdCommandList& commandList, WdFenceHandle fenceToSignal) = 0;

            virtual WdFormat findSupportedHardwareFormat(const std::vector<WdFormat>& formatCandidates, WdImageTiling tiling, WdFormatFeatureFlags features) = 0;

            virtual void transitionImageUsage(const WdImage& image, WdImageUsage currentUsage, WdImageUsage nextUsage) = 0;

            virtual void displayCurrentTarget() = 0;

        protected:
            static WdImage* create2DImagePtr(const std::vector<WdImageHandle>& _handles, const std::vector<WdMemoryHandle>& _memories, const std::vector<WdRenderTarget>& _targets, WdFormat _format, WdExtent2D _extent, WdImageUsageFlags _usage, WdClearValue _clearValue);
            static WdBuffer* createBufferPtr(const std::vector<WdBufferHandle>& _handles, const std::vector<WdMemoryHandle>& _memories, WdSize _size, WdBufferUsageFlags _usage);
            static WdImage* createTemp2DImagePtr();
            static WdBuffer* createTempBufferPtr();
            
            static WdResourceID globalResourceID;
            static WdResourceID globalTempResourceID;
    };   
}
#endif