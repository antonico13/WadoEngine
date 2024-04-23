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
            // Utils as static consts here for now 
            static const uint32_t FRAMES_IN_FLIGHT = 3;
            static const int CURRENT_FRAME_RESOURCE = -1;
            static const uint32_t TEMP_RESOURCE_BARRIER = 0x3FFFFFFF;

            // Returns pointer to a WdImage that represents the screen, can only be used as a fragment output!
            virtual Memory::WdClonePtr<WdImage> getDisplayTarget() = 0;
            
            virtual Memory::WdClonePtr<WdImage> create2DImage(WdExtent2D extent, uint32_t mipLevels, 
                    WdSampleCount sampleCount, WdFormat imageFormat, WdImageUsageFlags usageFlags, bool multiFrame = false) = 0;

            virtual Memory::WdClonePtr<WdBuffer> createBuffer(WdSize size, WdBufferUsageFlags usageFlags, bool multiFrame = false) = 0;
            // Temporary resources, these are *only* used for pipeline synchronisation creation, the pipeline
            // parameters that have been set to these objects need to be set to concrete objects before rendering.
            virtual Memory::WdClonePtr<WdImage> createTemp2DImage(bool multiFrame = false) = 0;

            virtual Memory::WdClonePtr<WdBuffer> createTempBuffer(bool multiFrame = false) = 0;

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