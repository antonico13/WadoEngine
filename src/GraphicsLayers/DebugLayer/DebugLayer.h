#ifndef WADO_DEBUG_LAYER_H
#define WADO_DEBUG_LAYER_H

#include "MainClonePtr.h"
#include "WdLayer.h"

namespace Wado::GAL::Debug {
    class DebugLayer : public WdLayer {
        public:
            // Returns ref to a WdImage that represents the screen, can only be used as a fragment output!
            Memory::WdClonePtr<WdImage> getDisplayTarget() override;

            Memory::WdClonePtr<WdImage> create2DImage(WdExtent2D extent, uint32_t mipLevels, WdSampleCount sampleCount, WdFormat imageFormat, WdImageUsageFlags usageFlags, bool multiFrame = false) override;
            
            WdFormat findSupportedHardwareFormat(const std::vector<WdFormat>& formatCandidates, WdImageTiling tiling, WdFormatFeatureFlags features) override;

            Memory::WdClonePtr<WdBuffer> createBuffer(WdSize size, WdBufferUsageFlags usageFlags, bool multiFrame = false) override;
            
            Memory::WdClonePtr<WdImage> createTemp2DImage(bool multiFrame = false) override;

            Memory::WdClonePtr<WdBuffer> createTempBuffer(bool multiFrame = false) override;

            WdSamplerHandle createSampler(const WdTextureAddressMode& addressMode = DEFAULT_TEXTURE_ADDRESS_MODE, WdFilterMode minFilter = WdFilterMode::WD_LINEAR, WdFilterMode magFilter = WdFilterMode::WD_LINEAR, WdFilterMode mipMapFilter = WdFilterMode::WD_LINEAR) override;
            
            void updateBuffer(const WdBuffer& buffer, void *data, WdSize offset, WdSize dataSize, int bufferIndex = CURRENT_FRAME_RESOURCE) override;
            void openBuffer(WdBuffer& buffer, int bufferIndex = CURRENT_FRAME_RESOURCE) override;
            void closeBuffer(WdBuffer& buffer, int bufferIndex = CURRENT_FRAME_RESOURCE) override;

            // Immediate functions
            void copyBufferToImage(const WdBuffer& buffer, const WdImage& image, WdExtent2D extent, int resourceIndex = CURRENT_FRAME_RESOURCE) override;
            void copyBuffer(const WdBuffer& srcBuffer, const WdBuffer& dstBuffer, WdSize size, int bufferIndex = CURRENT_FRAME_RESOURCE) override;

            WdFenceHandle createFence(bool signaled = true) override;
            void waitForFences(const std::vector<WdFenceHandle>& fences, bool waitAll = true, uint64_t timeout = UINT64_MAX) override;
            void resetFences(const std::vector<WdFenceHandle>& fences) override;
            
            Memory::WdClonePtr<WdPipeline> createPipeline(const Shader::WdShader& vertexShader, const Shader::WdShader& fragmentShader, const WdViewportProperties& viewportProperties) override;
            Memory::WdClonePtr<WdRenderPass> createRenderPass(const std::vector<WdPipeline>& pipelines) override;

            Memory::WdClonePtr<WdCommandList> createCommandList();

            void executeCommandList(const WdCommandList& commandList, WdFenceHandle fenceToSignal) override;

            void transitionImageUsage(const WdImage& image, WdImageUsage currentUsage, WdImageUsage nextUsage) override;

            void displayCurrentTarget() override;
        private:
            std::vector<Memory::WdMainPtr<WdCommandList>> _commandLists;
    };
};

#endif