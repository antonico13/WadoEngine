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
    class VulkanLayer : public GraphicsLayer {
        public:
            WdImage VulkanLayer::create2DImage(WdExtent2D extent, uint32_t mipLevels, WdSampleCount sampleCount, WdFormat imageFormat, WdImageUsageFlags usageFlags) override;
            static std::shared_ptr<VulkanLayer> getVulkanLayer();
        private:
            VulkanLayer();
            static std::shared_ptr<VulkanLayer> layer;

            // used for global sampler and texture creation, based on device
            // properties and re-calculated every time device is set up.
            static bool enableAnisotropy;
            static float maxAnisotropy;
            static uint32_t maxMipLevels;

            // the pointer management here will need to change 
            static std::vector<WdImage*> liveImages;
            static std::vector<WdBuffer*> liveBuffers;
            static std::vector<VkSampler> liveSamplers;
            static std::vector<VkFence> liveFences;
            static std::vector<VkSemaphore> liveSemaphores;
            static std::vector<VkCommandPool> liveCommandPools;
            static std::vector<VkPipeline> livePipelines;
            static std::vector<VkRenderPass> liveRenderPasses;
            
            
    };
}

#endif