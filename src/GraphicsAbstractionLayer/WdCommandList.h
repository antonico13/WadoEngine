#ifndef WADO_GRAPHICS_COMMAND_LIST
#define WADO_GRAPHICS_COMMAND_LIST

#include "WdCommon.h"
#include "WdImage.h"
#include "WdBuffer.h"

#include <vector>

namespace Wado::GAL { 
    // Forward declaration
    class WdRenderPass;
    // Forward declarations 
    class WdRenderPassInfo;
    class WdImageBlitInfo;

    enum WdImageLayout {

    };

    class WdClearRegion;

    using WdRenderTargetHandle = uint64_t;

    // 1-to-1 with Vulkan
    enum WdImageAspect {
        WD_IMAGE_ASPECT_COLOR_BIT = 0x00000001,
        WD_IMAGE_ASPECT_DEPTH_BIT = 0x00000002,
        WD_IMAGE_ASPECT_STENCIL_BIT = 0x00000004,
        WD_IMAGE_ASPECT_METADATA_BIT = 0x00000008,
    };

    using WdImageAspectFlags = uint64_t;

    // TODO: this could be done in a nicer way, don't need to follow vulkan here with all the unions 
    using WdRenderTargetClearInfo = struct WdRenderTargetClearInfo {
        WdRenderTargetHandle handle; // render target to clear
        WdImageAspectFlags aspectFlags; // aspects of the render target to clear 
        WdClearValue clearValue;
    };

    using WdColorImageClearInfo = struct WdColorImageClearInfo {
        WdColorValue colorClearValue;
        std::vector<WdClearRegion> clearRegions;
    };

    using WdDepthStencilClearInfo = struct WdDepthStencilClearInfo {
        WdDepthStencilValue depthStencilClearValue;
        std::vector<WdClearRegion> clearRegions;
    };

    using WdBufferCopyRegion = struct WdBufferCopyRegion {
        WdSize srcBufferOffset;
        WdSize dstBufferOffset;
        WdSize copySize;
    };

    using WdImageCopyRegion = struct WdImageCopyRegion {
        WdExtent2D srcOffset;
        WdExtent2D dstOffset;
        WdExtent2D copyExtent;
    };

    using WdBufferImageCopyRegion = struct WdBufferImageCopyRegion {
        WdSize bufferOffset;
        WdExtent2D bufferRange;
        WdExtent2D imageOffset;
        WdExtent2D imageRange;
    };

    using WdBlendConstant = struct WdBlendConstant {
        float r_c;
        float g_c;
        float b_c;
        float a_c;
    };

    using WdDepthBias = struct WdDepthBias {
        float depthBiasAddFactor;
        float depthBiasClamp;
        float depthBiasSlopeFactor;
    };

    using WdDepthBounds = struct WdDepthBounds {
        float minDepth;
        float maxDepth;
    };

    using WdRect2D = struct WdRect2D {
        WdExtent2D offset;
        WdExtent2D extent;
    };

    using WdViewport = struct WdViewport {
        WdRect2D region;
        WdDepthBounds depthBounds;
    };

    enum WdStencilFace {
        WD_FACE_FRONT = 0x01,
        WD_FACE_BACK = 0x02,
    };

    using WdStencilFaceFlags = uint64_t;

    using WdPipelineState = struct WdPipelineState {

    };

    using WdShaderParamInfo = struct WdShaderParamInfo {

    };

    // TODO: for all images, should specify subresource ranges somehow? 
    // These are supported by every type of command list 
    class WdCommandListBase {
        public:
            virtual void copyBuffer(const WdBuffer& srcBuffer, const WdBuffer& dstBuffer, const std::vector<WdBufferCopyRegion>& copyRegions) = 0;
            virtual void copyBufferToImage(const WdBuffer& srcBuffer, const WdImage& dstImage, WdImageLayout imageLayout, const std::vector<WdBufferImageCopyRegion>& copyRegions) = 0;
            virtual void copyImage(const WdImage& srcImage, const WdImage& dstImage, WdImageLayout srcLayout, WdImageLayout dstLayout, const std::vector<WdImageCopyRegion>& copyRegions) = 0;
            virtual void copyImageToBuffer(const WdImage& srcImage, WdImageLayout imageLayout, const WdBuffer& dstBuffer, const std::vector<WdBufferImageCopyRegion>& copyRegions) = 0;
            virtual void fillBuffer(const WdBuffer& dstBuffer, WdSize offset, WdSize size, uint32_t value) = 0;
            // Only use with small sizes 
            virtual void updateBuffer(const WdBuffer& dstBuffer, WdSize offset, WdSize updateSize, const void* updateData) = 0;
    }; 

    class WdComputeCommandList : public WdCommandListBase {
        public:
            virtual void clearColorImage(const WdImage& image, const WdImageLayout& currentLayout, const WdColorImageClearInfo& colorClearInfo) = 0;
            virtual void dispatchCompute(WdSize groupCountX, WdSize groupCountY, WdSize groupCountZ) = 0;
            virtual void dispatchComputeIndirect(const WdBuffer& buffer, WdSize offset) = 0;

    };

    class WdTransferCommandList : public WdCommandListBase {
        public:

    };
    
    // The graphics command list is the strongest one 
    class WdGraphicsCommandList : public WdComputeCommandList {
        public:
            virtual void beginRenderPass(const WdRenderPassInfo& renderPassInfo) = 0;
            virtual void endRenderPass() = 0;
            virtual void nextSubPass() = 0;

            virtual void setBlendConstant(const WdBlendConstant& blendConstant) = 0;
            virtual void setDepthBias(const WdDepthBias& depthBias) = 0;
            virtual void setDepthBounds(const WdDepthBounds& depthBounds) = 0;
            // TODO: in what case are these arrays?
            virtual void setScissors(const std::vector<WdRect2D>& scissors) = 0;
            virtual void setViewports(const std::vector<WdViewport>& viewports) = 0;

            virtual void setPipelineState(const WdPipelineState& state) = 0;
            virtual void setShaderParams(const std::vector<WdShaderParamInfo>& params) = 0;

            virtual void setStencilCompareMask(WdStencilFaceFlags faceFlags, uint32_t compareMask) = 0;
            virtual void setStencilReference(WdStencilFaceFlags faceFlags, uint32_t reference) = 0;
            virtual void setStencilWriteMask(WdStencilFaceFlags faceFlags, uint32_t writeMask) = 0;

            virtual void setIndexBuffer(const WdBuffer& indexBuffer) = 0;
            virtual void setVertexBuffers(const std::vector<WdBuffer>& vertexBuffers) = 0;

            virtual void drawVertices(WdSize vertexCount, WdSize firstVertex = 0, WdSize instanceCount = 0, WdSize firstInstance = 0) = 0;
            virtual void drawVerticesIndirect(const WdBuffer& buffer, WdSize bufferOffset = 0, WdSize inputStride = 0, WdSize drawCount = 0) = 0;

            virtual void drawIndexed(WdSize indexCount, WdSize firstIndex = 0, WdSize indexOffset = 0, WdSize instanceCount = 0, WdSize firstInstance = 0) = 0;
            virtual void drawIndexedIndirect(const WdBuffer& buffer, WdSize bufferOffset = 0, WdSize inputStride = 0, WdSize drawCount = 0) = 0;
        
            // Only outside of render passes 
            virtual void blitImage(const WdImage& srcImag                     e, const WdImage& dstImage, WdImageLayout srcLayout, WdImageLayout dstLayout, WdFilterMode scaleFilter, const std::vector<WdImageBlitInfo>& blitRegions) = 0;
            virtual void resolveImage(const WdImage& srcImage, WdImageLayout srcLayout, const WdImage& dstImage, WdImageLayout dstLayout, const std::vector<WdImageCopyRegion>& resolveRegions) = 0;

            virtual void clearRenderTargets(const std::vector<WdRenderTargetClearInfo>& renderTargetClearInfos, const std::vector<WdClearRegion>& perRenderTargetClearRegions) = 0;
            virtual void clearDepthStencil(const WdImage& image, const WdImageLayout& currentLayout, const WdDepthStencilClearInfo& depthStencilClearInfo) = 0;
    };
};
#endif