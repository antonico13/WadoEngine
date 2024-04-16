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
            WdImage& create2DImage(WdExtent2D extent, uint32_t mipLevels, WdSampleCount sampleCount, WdFormat imageFormat, WdImageUsageFlags usageFlags) override;
            WdBuffer& createBuffer(WdSize size, WdBufferUsageFlags usageFlags) override;
            WdSamplerHandle createSampler(WdTextureAddressMode addressMode = DefaultTextureAddressMode, WdFilterMode minFilter = WdFilterMode::WD_LINEAR, WdFilterMode magFilter = WdFilterMode::WD_LINEAR, WdFilterMode mipMapFilter = WdFilterMode::WD_LINEAR) override;
            
            void updateBuffer(const WdBuffer& buffer, void *data, WdSize offset, WdSize dataSize) override;
            void openBuffer(WdBuffer& buffer) override;
            void closeBuffer(WdBuffer& buffer) override;

            WdFenceHandle createFence(bool signaled = true) override;
            void waitForFences(const std::vector<WdFenceHandle>& fences, bool waitAll = true, uint64_t timeout = UINT64_MAX) override;
            void resetFences(const std::vector<WdFenceHandle>& fences) override;
            
            WdPipeline createPipeline(Shader::Shader vertexShader, Shader::Shader fragmentShader, WdVertexBuilder* vertexBuilder, WdViewportProperties viewportProperties) override;
            WdRenderPass createRenderPass(const std::vector<WdPipeline>& pipelines) override;

            static std::shared_ptr<VulkanLayer> getVulkanLayer();
        private:
            VulkanLayer();
            static std::shared_ptr<VulkanLayer> _layer;

            // Internal components 
            VkPhysicalDevice _physicalDevice = VK_NULL_HANDLE;
            VkDevice _device;

            static VkFormat WdFormatToVkFormat[] = {
                VK_FORMAT_UNDEFINED,
                VK_FORMAT_R8_UNORM,
                VK_FORMAT_R8_SINT,
                VK_FORMAT_R8_UINT,
                VK_FORMAT_R8G8_UNORM,
                VK_FORMAT_R8G8_SINT,
                VK_FORMAT_R8G8_UINT,
                VK_FORMAT_R8G8B8_UNORM,
                VK_FORMAT_R8G8B8_SINT,
                VK_FORMAT_R8G8B8_UINT,
                VK_FORMAT_R8G8B8A8_UNORM,
                VK_FORMAT_R8G8B8A8_SINT,
                VK_FORMAT_R8G8B8A8_UINT,
                VK_FORMAT_R16_SINT,
                VK_FORMAT_R16_UINT,
                VK_FORMAT_R16_SFLOAT,
                VK_FORMAT_R16G16_SINT,
                VK_FORMAT_R16G16_UINT,
                VK_FORMAT_R16G16_SFLOAT,
                VK_FORMAT_R16G16B16_SINT,
                VK_FORMAT_R16G16B16_UINT,
                VK_FORMAT_R16G16B16_SFLOAT,
                VK_FORMAT_R16G16B16A16_SINT,
                VK_FORMAT_R16G16B16A16_UINT,
                VK_FORMAT_R16G16B16A16_SFLOAT,
                VK_FORMAT_R32_SINT,
                VK_FORMAT_R32_UINT,
                VK_FORMAT_R32_SFLOAT,
                VK_FORMAT_R32G32_SINT,
                VK_FORMAT_R32G32_UINT,
                VK_FORMAT_R32G32_SFLOAT,
                VK_FORMAT_R32G32B32_SINT,
                VK_FORMAT_R32G32B32_UINT,
                VK_FORMAT_R32G32B32_SFLOAT,
                VK_FORMAT_R32G32B32A32_SINT,
                VK_FORMAT_R32G32B32A32_UINT,
                VK_FORMAT_R32G32B32A32_SFLOAT,
                VK_FORMAT_R64_SINT,
                VK_FORMAT_R64_UINT,
                VK_FORMAT_R64_SFLOAT,
                VK_FORMAT_R64G64_SINT,
                VK_FORMAT_R64G64_UINT,
                VK_FORMAT_R64G64_SFLOAT,
                VK_FORMAT_R64G64B64_SINT,
                VK_FORMAT_R64G64B64_UINT,
                VK_FORMAT_R64G64B64_SFLOAT,
                VK_FORMAT_R64G64B64A64_SINT,
                VK_FORMAT_R64G64B64A64_UINT,
                VK_FORMAT_R64G64B64A64_SFLOAT,
            };

            // used for global sampler and texture creation, based on device
            // properties and re-calculated every time device is set up.
            bool _enableAnisotropy;
            float _maxAnisotropy;
            uint32_t _maxMipLevels;

            // the pointer management here will need to change 
            std::vector<WdImage*> _liveImages;
            std::vector<WdBuffer*> _liveBuffers;
            std::vector<VkSampler> _liveSamplers;
            std::vector<VkFence> _liveFences;
            std::vector<VkSemaphore> _liveSemaphores;
            std::vector<VkCommandPool> _liveCommandPools;
            std::vector<VkPipeline> _livePipelines;
            std::vector<VkRenderPass> _liveRenderPasses;

            // needed in order to determine resource sharing mode and queues to use
            const VkImageUsageFlags transferUsage = VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
            const VkImageUsageFlags graphicsUsage = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT | VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT;
            const VkImageUsageFlags presentUsage = 0;

            const VkBufferUsageFlags bufferTransferUsage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
            const VkBufferUsageFlags bufferGraphicsUsage = VK_BUFFER_USAGE_UNIFORM_TEXEL_BUFFER_BIT | VK_BUFFER_USAGE_STORAGE_TEXEL_BUFFER_BIT | VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
            const VkBufferUsageFlags bufferIndirectUsage = VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT;

            QueueFamilyIndices _queueIndices;

            VkSampleCountFlagBits WdSampleBitsToVkSampleBits(WdSampleCount sampleCount) const;

            VkImageUsageFlags WdImageUsageToVkImageUsage(WdImageUsageFlags imageUsage) const;
            VkBufferUsageFlags WdBufferUsageToVkBufferUsage(WdBufferUsageFlags bufferUsage) const;

            std::vector<uint32_t> getImageQueueFamilies(VkImageUsageFlags usage) const;
            std::vector<uint32_t> getBufferQueueFamilies(VkBufferUsageFlags usage) const;

            VkImageAspectFlags getImageAspectFlags(VkImageUsageFlags usage) const;

            VkFilter WdFilterToVkFilter(WdFilterMode filter) const;
            VkSamplerAddressMode WdAddressModeToVkAddressMode(WdAddressMode addressMode) const;
    };

    class VulkanRenderPass : public WdRenderPass {
        public:
            friend class GraphicsLayer;
        private:
            using VulkanPipeline = struct VulkanPipeline {
                VkPipeline pipeline;
                VkPipelineLayout pipelineLayout;
                VkDescriptorSetLayout descriptorSetLayout;
                VkDescriptorSet descriptorSet;
            };

            VulkanRenderPass(std::vector<WdPipeline> pipelines, GraphicsLayer* vulkanLayer);
            void init() override;
            VulkanPipeline createVulkanPipeline(WdPipeline pipeline, uint8_t index);

            GraphicsLayer* _vulkanLayer;
            
            VkRenderPass _renderPass;
            VkDescriptorPool _descriptorPool;
            
            std::vector<WdPipeline> _pipelines
            std::vector<VulkanPipeline> _vkPipelines;
            
    };
}

#endif