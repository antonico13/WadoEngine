#ifndef WADO_GRAPHICS_ABSTRACTION_LAYER
#define WADO_GRAPHICS_ABSTRACTION_LAYER

#include "WdCommon.h"
#include "WdImage.h"
#include "WdBuffer.h"
#include "WdPipeline.h"
#include "WdCommandList.h"

#include "Shader.h"

#include "MainClonePtr.h"

#include <vector>

namespace Wado::GAL {
    // Graphics abstraction layer.
    // Backends such as Vulkan or Direct X implement this interface,
    // and all rendering code should be written using these functions
    class WdLayer {
        public:
            // Returns pointer to a WdImage that represents the screen, can only be used as a fragment output!
            virtual Memory::WdClonePtr<WdImage> getDisplayTarget() = 0;
            
            virtual Memory::WdClonePtr<WdImage> create2DImage(WdExtent2D extent, uint32_t mipLevels, 
                    WdSampleCount sampleCount, WdFormat imageFormat, WdImageUsageFlags usageFlags) = 0;

            virtual Memory::WdClonePtr<WdBuffer> createBuffer(WdSize size, WdBufferUsageFlags usageFlags) = 0;

            // create texture sampler 
            virtual WdSamplerHandle createSampler(const WdTextureAddressMode& addressMode = DEFAULT_TEXTURE_ADDRESS_MODE, WdFilterMode minFilter = WdFilterMode::WD_LINEAR, WdFilterMode magFilter = WdFilterMode::WD_LINEAR, WdFilterMode mipMapFilter = WdFilterMode::WD_LINEAR) = 0;

            virtual void updateBuffer(const WdBuffer& buffer, void *data, WdSize offset, WdSize dataSize) = 0;
            // close or open "pipe" between CPU and GPU for this buffer's memory
            virtual void openBuffer(WdBuffer& buffer) = 0;

            virtual void closeBuffer(WdBuffer& buffer) = 0;

            // immediate versions 
            virtual void copyBufferToImage(const WdBuffer& buffer, const WdImage& image, WdExtent2D extent) = 0;
            virtual void copyBuffer(const WdBuffer& srcBuffer, const WdBuffer& dstBuffer, WdSize size) = 0;

            virtual WdFenceHandle createFence(bool signaled = true) = 0;

            virtual void waitForFences(const std::vector<WdFenceHandle>& fences, bool waitAll = true, uint64_t timeout = UINT64_MAX) = 0;

            virtual void resetFences(const std::vector<WdFenceHandle>& fences) = 0;

            virtual Memory::WdClonePtr<WdPipeline> createPipeline(const Shader::WdShader& vertexShader, const Shader::WdShader& fragmentShader, const WdViewportProperties& viewportProperties) = 0;

            virtual Memory::WdClonePtr<WdRenderPass> createRenderPass(const std::vector<WdPipeline>& pipelines) = 0;

            virtual Memory::WdClonePtr<WdCommandList> createCommandList() = 0;

            virtual void executeCommandList(const WdCommandList& commandList) = 0;

            virtual WdFormat findSupportedHardwareFormat(const std::vector<WdFormat>& formatCandidates, WdImageTiling tiling, WdFormatFeatureFlags features) = 0;

            virtual void transitionImageUsage(const WdImage& image, WdImageUsage currentUsage, WdImageUsage nextUsage) = 0;

            virtual void displayCurrentTarget() = 0;

        protected:
            static WdImage* create2DImagePtr(WdImageHandle _handle, WdMemoryHandle _memory, WdRenderTarget _target, WdFormat _format, WdExtent3D _extent, WdImageUsageFlags _usage, WdClearValue _clearValue);
            static WdBuffer* createBufferPtr(WdBufferHandle _handle, WdMemoryHandle _memory, WdSize _size, WdBufferUsageFlags _usage);
    };   
}
#endif