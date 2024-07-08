#ifndef WADO_VULKAN_COMMAND_LIST
#define WADO_VULKAN_COMMAND_LIST

#include "vulkan/vulkan.h"

#include "WdRenderPass.h" // Not sure if this include is needed 
#include "WdCommandList.h"

#include "VulkanRenderPass.h"

namespace Wado::GAL::Vulkan {

    class VulkanCommandList : public WdCommandList {
        public:
            friend class VulkanLayer;

            void resetCommandList() override;
            void beginCommandList() override;
            void setRenderPass(const WdRenderPass& renderPass) override;
            void nextPipeline() override;
            void setVertexBuffers(const std::vector<WdBuffer>& vertexBuffer) override;
            void setIndexBuffer(const WdBuffer& indexBuffer) override;
            void setViewport(const WdViewportProperties& WdViewportProperties) override;
            void drawIndexed(uint32_t indexCount) override;
            void drawVertices(uint32_t vertexCount) override;
            void endRenderPass() override;
            void endCommandList() override;
            // non-immediate versions 
            void copyBufferToImage(const WdBuffer& buffer, const WdImage& image, WdExtent2D extent) override;
            void copyBuffer(const WdBuffer& srcBuffer, const WdBuffer& dstBuffer, WdSize size) override;
        private:
            VulkanCommandList(const std::vector<VkCommandBuffer>& graphicsBuffers, const std::vector<VkCommandBuffer>& transferBuffers);

            const std::vector<VkCommandBuffer>& _graphicsCommandBuffers; // per frame in flight 
            const std::vector<VkCommandBuffer>& _transferCommandBuffers; // per frame in flight

            std::vector<VulkanRenderPass::VulkanPipelineInfo>::const_iterator _startingPipeline;
            std::vector<VulkanRenderPass::VulkanPipelineInfo>::const_iterator _currentPipeline;
            std::vector<VulkanRenderPass::VulkanPipelineInfo>::const_iterator _endingPipeline;
    };
};

#endif