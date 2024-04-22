#include "VulkanCommandList.h"

namespace Wado::GAL::Vulkan {

    void VulkanCommandList::resetCommandList() { 
        if (vkResetCommandBuffer(_graphicsCommandBuffer, 0) != VK_SUCCESS) { // TOOD: when should I release resources back to the command pool? the reset flag controls this 
            throw std::runtime_error("Failed to reset Vulkan command list!");
        }
    };
    
    void VulkanCommandList::beginCommandList() { 
        // TODO: should throw error here if command buffer has already been started and not reset?
        VkCommandBufferBeginInfo beginInfo{};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        beginInfo.flags = 0; // TODO: for this one since it's graphics, no flags, but for transfer and present i think this should be set 
        beginInfo.pInheritanceInfo = nullptr; // TODO: this 

        if (vkBeginCommandBuffer(_graphicsCommandBuffer, &beginInfo) != VK_SUCCESS) {
            throw std::runtime_error("Failed to begin recording Vulkan command list!");
        }
    };
    
    void VulkanCommandList::setRenderPass(const WdRenderPass& renderPass) { 
        // TODO: This has to be safe every time, need to ensure with destruction of all objects 
        // when switching backend layers.
        // TODO: maybe add assert here?
        const VulkanRenderPass& _vkRenderPass = reinterpret_cast<const VulkanRenderPass&>(renderPass);

        VkRenderPassBeginInfo renderPassInfo{};
        renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        renderPassInfo.renderPass = _vkRenderPass._renderPass;
        renderPassInfo.framebuffer = _vkRenderPass._framebuffer.framebuffer; // TODO: how to handle multiple frames in flight here?
        renderPassInfo.renderArea.offset = _vkRenderPass._renderArea.offset;
        renderPassInfo.renderArea.extent = _vkRenderPass._renderArea.extent;

        renderPassInfo.clearValueCount = static_cast<uint32_t>(_vkRenderPass._framebuffer.clearValues.size());
        renderPassInfo.pClearValues = _vkRenderPass._framebuffer.clearValues.data();

        vkCmdBeginRenderPass(_graphicsCommandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE); // TODO: this might not always be inline? Not sure 

        _currentPipeline = _vkRenderPass._vkPipelines.begin();
        _startingPipeline = _vkRenderPass._vkPipelines.begin();
        _endingPipeline = _vkRenderPass._vkPipelines.end();
    };
    
    void VulkanCommandList::nextPipeline() { 
        // TODO: should throw error here if render pass not set, if it's last pipeline, etc 
        if (_currentPipeline != _startingPipeline) {
            vkCmdNextSubpass(_graphicsCommandBuffer, VK_SUBPASS_CONTENTS_INLINE);
        };

        vkCmdBindPipeline(_graphicsCommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, _currentPipeline->pipeline);
 
        // TODO: binding multiple decsriptor sets here?
        vkCmdBindDescriptorSets(_graphicsCommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, _currentPipeline->pipelineLayout, 0, 1, &_currentPipeline->descriptorSet, 0, nullptr); // TODO: handle multi-frame here 
        
        _currentPipeline++;
    };
    
    void VulkanCommandList::setVertexBuffers(const std::vector<WdBuffer>& vertexBuffers) { 
        //TODO: Assuming all offsets are 0 for now 
        std::vector<VkBuffer> vkBuffers(vertexBuffers.size());
        std::vector<VkDeviceSize> offsets(vertexBuffers.size());

        for (const WdBuffer& wdBuffer : vertexBuffers) {
            vkBuffers.push_back(static_cast<VkBuffer>(wdBuffer.handle));
            offsets.push_back(0);
        };

        // TODO: look into if handling multiple vertex bindings is needed 
        vkCmdBindVertexBuffers(_graphicsCommandBuffer, 0, static_cast<uint32_t>(vkBuffers.size()), vkBuffers.data(), offsets.data());
    };
    
    void VulkanCommandList::setIndexBuffer(const WdBuffer& indexBuffer) { 
        vkCmdBindIndexBuffer(_graphicsCommandBuffer, static_cast<VkBuffer>(indexBuffer.handle), 0, VK_INDEX_TYPE_UINT32); // TODO: look into if I need to address the offset 
    };
    
    void VulkanCommandList::setViewport(const WdViewportProperties& viewportProperties) { 
        VkViewport viewport{};
        viewport.x = static_cast<float>(viewportProperties.startCoords.width);
        viewport.y = static_cast<float>(viewportProperties.startCoords.height);
        viewport.width = static_cast<float>(viewportProperties.endCoords.width);
        viewport.height = static_cast<float>(viewportProperties.endCoords.height);
        viewport.minDepth = viewportProperties.depth.min;
        viewport.maxDepth = viewportProperties.depth.max;

        vkCmdSetViewport(_graphicsCommandBuffer, 0, 1, &viewport); // TODO: support only one right now 

        VkRect2D scissor{};
        scissor.offset = {static_cast<int32_t>(viewportProperties.scissor.offset.width), static_cast<int32_t>(viewportProperties.scissor.offset.height)};
        scissor.extent = {viewportProperties.scissor.extent.width, viewportProperties.scissor.extent.height};

        vkCmdSetScissor(_graphicsCommandBuffer, 0, 1, &scissor);
    };
    
    void VulkanCommandList::drawIndexed(uint32_t indexCount) { 
        // TODO: throw error if draw is called without setting viewport, index, vertex buffer etc first 
        vkCmdDrawIndexed(_graphicsCommandBuffer, indexCount, 1, 0, 0, 0); // TODO for all of the params, idk if they should be customizable or not 
    };
    
    void VulkanCommandList::drawVertices(uint32_t vertexCount) { 
        // TODO: throw error if draw is called without setting viewport first 
        // TODO: throw error if trying to call this with index buffer set 
        vkCmdDraw(_graphicsCommandBuffer, vertexCount, 1, 0, 0); // TODO: as with draw index, find out if these params should be user-set
    };
    
    void VulkanCommandList::endRenderPass() { 
        // TODO: check if all subpasses have finished, need to call this only after submitting last subpass
        vkCmdEndRenderPass(_graphicsCommandBuffer);
    };
    
    void VulkanCommandList::endCommandList() { 
        if (vkEndCommandBuffer(_graphicsCommandBuffer) != VK_SUCCESS) {
            throw std::runtime_error("Failed to end Vulkan command list!");
        };
    };

    // non-immediate versions 
    void VulkanCommandList::copyBufferToImage(const WdBuffer& buffer, const WdImage& image, WdExtent2D extent) {
        // TODO
    };
    
    void VulkanCommandList::copyBuffer(const WdBuffer& srcBuffer, const WdBuffer& dstBuffer, WdSize size) {
        // TODO
    };

    // Private constructor
    VulkanCommandList::VulkanCommandList(VkCommandBuffer graphicsBuffer) : _graphicsCommandBuffer(graphicsBuffer) { };

};