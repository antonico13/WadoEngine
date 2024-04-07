#ifndef H_WD_VULKAN_LAYER
#define H_WD_VULKAN_LAYER

#define GLFW_INCLUDE_VULKAN
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_ENABLE_EXPERIMENTAL
#include <GLFW/glfw3.h>


#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/hash.hpp>

#include "GraphicsAbstractionLayer.h"
#include <memory>

namespace Wado::GAL::Vulkan {


    struct QueueFamilyIndices {
        std::optional<uint32_t> graphicsFamily;
        std::optional<uint32_t> presentFamily;
        std::optional<uint32_t> transferFamily;
        //std::optional<uint32_t> computeFamily;

        bool isComplete();
    };

    class VulkanLayer : public GraphicsLayer {
        public:
            WdImage create2DImage(WdExtent2D extent, uint32_t mipLevels, WdSampleCount sampleCount, WdFormat imageFormat, WdImageUsageFlags usageFlags) override;
            WdBuffer createBuffer(WdSize size, WdBufferUsageFlags usageFlags) override;
            static std::shared_ptr<VulkanLayer> getVulkanLayer();
        private:
            VulkanLayer();
            static std::shared_ptr<VulkanLayer> layer;

            static VkFormat WdFormatToVKFormat[] = {
                VK_FORMAT_R8G8B8A8_UINT,
                VK_FORMAT_R8G8B8A8_SINT,
                VK_FORMAT_R8G8B8A8_SRGB,
                VK_FORMAT_UNDEFINED,
                VK_FORMAT_R32G32B32A32_UINT,
                VK_FORMAT_R32G32B32A32_SINT,
                VK_FORMAT_R32G32B32A32_SFLOAT,
                VK_FORMAT_R32G32B32_SFLOAT,
                VK_FORMAT_R32G32_SFLOAT,
                VK_FORMAT_R32_SINT,
                VK_FORMAT_D32_SFLOAT,
                VK_FORMAT_D32_SFLOAT_S8_UINT,
                VK_FORMAT_D24_UNORM_S8_UINT,
            };

            // used for global sampler and texture creation, based on device
            // properties and re-calculated every time device is set up.
            bool enableAnisotropy;
            float maxAnisotropy;
            uint32_t maxMipLevels;

            // the pointer management here will need to change 
            std::vector<WdImage*> liveImages;
            std::vector<WdBuffer*> liveBuffers;
            std::vector<VkSampler> liveSamplers;
            std::vector<VkFence> liveFences;
            std::vector<VkSemaphore> liveSemaphores;
            std::vector<VkCommandPool> liveCommandPools;
            std::vector<VkPipeline> livePipelines;
            std::vector<VkRenderPass> liveRenderPasses;

            // needed in order to determine resource sharing mode and queues to use
            const VkImageUsageFlags transferUsage = VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
            const VkImageUsageFlags graphicsUsage = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT | VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT;
            const VkImageUsageFlags presentUsage = 0;

            const VkBufferUsageFlags bufferTransferUsage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
            const VkBufferUsageFlags bufferGraphicsUsage = VK_BUFFER_USAGE_UNIFORM_TEXEL_BUFFER_BIT | VK_BUFFER_USAGE_STORAGE_TEXEL_BUFFER_BIT | VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
            const VkBufferUsageFlags bufferIndirectUsage = VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT;

            QueueFamilyIndices queueIndices;

            VkSampleCountFlagBits WdSampleBitsToVkSampleBits(WdSampleCount sampleCount) const;

            VkImageUsageFlags WdImageUsageToVkImageUsage(WdImageUsageFlags imageUsage) const;
            VkBufferUsageFlags WdBufferUsageToVkBufferUsage(WdBufferUsageFlags bufferUsage) const;

            std::vector<uint32_t> getImageQueueFamilies(VkImageUsageFlags usage) const;
            std::vector<uint32_t> getBufferQueueFamilies(VkBufferUsageFlags usage) const;

            VkImageAspectFlags getImageAspectFlags(VkImageUsageFlags usage) const;
    };
}

#endif