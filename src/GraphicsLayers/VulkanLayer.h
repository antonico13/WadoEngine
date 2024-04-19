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
            // Returns ref to a WdImage that represents the screen, can only be used as a fragment output!
            WdImage& getDisplayTarget() override;
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
            VulkanLayer(GLFWwindow* window, bool debugEnabled);
            void init();
            
            static std::shared_ptr<VulkanLayer> _layer;

            // Init functions 
            void createInstance();
            void createSurface();
            void setupDebugMessenger();
            void pickPhysicalDevice();
            void createLogicalDevice();
            void createGraphicsCommandPool();
            void createTransferCommandPool();
            void createSwapchain();

            // Init utils

            static const std::vector<const char*> _validationLayers = {
                "VK_LAYER_KHRONOS_validation",
                "VK_LAYER_LUNARG_monitor"
            };

            static const std::vector<const char*> _deviceExtensions = {
                VK_KHR_SWAPCHAIN_EXTENSION_NAME/*,
                VK_NV_DEVICE_DIAGNOSTIC_CHECKPOINTS_EXTENSION_NAME,*/
            };

            using SwapChainSupportDetails = struct SwapChainSupportDetails {
                VkSurfaceCapabilitiesKHR capabilities;
                std::vector<VkSurfaceFormatKHR> formats;
                std::vector<VkPresentModeKHR> presentModes;
            };

            const bool _enableValidationLayers;

            uint32_t _extensionCount = 0;
            std::vector<VkExtensionProperties> _extensions;
            uint32_t _layerCount = 0;
            std::vector<VkLayerProperties> _layers;

            VkInstance _instance;
            GLFWwindow* _window;
            VkDebugUtilsMessengerEXT _debugMessenger;
            VkSurfaceKHR _surface;

            void getSupportedValidationLayers();
            void getSupportedExtensions();
            std::vector<const char *> getRequiredExtensions();
            bool checkStringSubset(const char** superSet, uint32_t superSetCount, const char** subSet, uint32_t subSetCount);
            bool checkRequiredExtensions(const std::vector<const char*>& requiredExtensions);
            bool checkRequiredLayers();
            bool isDeviceSuitable(VkPhysicalDevice device);
            bool checkDeviceExtensionSupport(VkPhysicalDevice device);
            SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice device);
            QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device);
            VkSampleCountFlagBits getMaxUsableSampleCount();

            void populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo);

            // Internal components 
            VkPhysicalDevice _physicalDevice = VK_NULL_HANDLE;
            VkDevice _device;
            VkSampleCountFlagBits _msaaSamples = VK_SAMPLE_COUNT_1_BIT;

            VkQueue _graphicsQueue;
            VkQueue _presentQueue;
            VkQueue _transferQueue;

            VkCommandPool _graphicsCommandPool;
            VkCommandPool _transferCommandPool;

            VkSwapchainKHR _swapchain;
            std::vector<VkImage> _swapchainImages;
            VkFormat _swapchainImageFormat;
            VkExtent2D _swapchainImageExtent;
            std::vector<VkImageView> _swapchainImageViews;

            static const VkFormat WdFormatToVkFormat[] = {
                VK_FORMAT_UNDEFINED,
                VK_FORMAT_R8_UNORM,
                VK_FORMAT_R8_SINT,
                VK_FORMAT_R8_UINT,
                VK_FORMAT_R8_SRGB,
                VK_FORMAT_R8G8_UNORM,
                VK_FORMAT_R8G8_SINT,
                VK_FORMAT_R8G8_UINT,
                VK_FORMAT_R8G8_SRGB,
                VK_FORMAT_R8G8B8_UNORM,
                VK_FORMAT_R8G8B8_SINT,
                VK_FORMAT_R8G8B8_UINT,
                VK_FORMAT_R8G8B8_SRGB,
                VK_FORMAT_R8G8B8A8_UNORM,
                VK_FORMAT_R8G8B8A8_SINT,
                VK_FORMAT_R8G8B8A8_UINT,
                VK_FORMAT_R8G8B8A8_SRGB,
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
            std::vector<VulkanRenderPass*> _liveRenderPasses;

            // needed in order to determine resource sharing mode and queues to use
            const VkImageUsageFlags transferUsage = VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
            const VkImageUsageFlags graphicsUsage = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT | VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT;
            const VkImageUsageFlags presentUsage = 0;

            const VkBufferUsageFlags bufferTransferUsage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
            const VkBufferUsageFlags bufferGraphicsUsage = VK_BUFFER_USAGE_UNIFORM_TEXEL_BUFFER_BIT | VK_BUFFER_USAGE_STORAGE_TEXEL_BUFFER_BIT | VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
            const VkBufferUsageFlags bufferIndirectUsage = VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT;

            QueueFamilyIndices _queueIndices;
            SwapChainSupportDetails _swapChainSupportDetails;

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
            friend class VulkanLayer;
            friend class VulkanCommandList;

        private:
            using VulkanPipeline = struct VulkanPipeline {
                VkPipeline pipeline;
                VkPipelineLayout pipelineLayout;
                std::vector<VkDescriptorSetLayout> descriptorSetLayouts;
                std::vector<VkDescriptorSet> descriptorSets;
            };

            using AttachmentInfo = struct AttachmentInfo {
                VkAttachmentDescription attachmentDesc;            
                std::vector<VkAttachmentReference> refs;
                uint8_t attachmentIndex;
                bool presentSrc;
            };

            using AttachmentMap = std::map<WdImageHandle, AttachmentInfo>;

            using ResourceInfo = struct ResourceInfo {
                WdPipeline::ShaderParamType type;
                WdStageMask stages;
                uint8_t pipelineIndex;
            };

            using ImageResources = std::map<WdImageHandle, std::vector<ResourceInfo>>;
            using BufferResources = std::map<WdBufferHandle, std::vector<ResourceInfo>>;

            using VertexInputDesc = std::tuple<std::vector<VkVertexInputAttributeDescription>, VkVertexInputBindingDescription>;

            using Framebuffer = struct Framebuffer {
                std::vector<VkImageView> attachmentViews;
                std::vector<VkClearValue> clearValues;
                VkFramebuffer framebuffer;
            };

            using RenderArea = struct RenderArea {
                WdExtent2D offset;
                WdExtent2D extent;
            };

            static const uint32_t _imgReadMask = WdPipeline::ShaderParameterType::WD_SAMPLED_IMAGE & WdPipeline::ShaderParameterType::WD_TEXTURE_IMAGE & WdPipeline::ShaderParameterType::WD_STORAGE_IMAGE & WdPipeline::ShaderParameterType::WD_SUBPASS_INPUT;
            static const uint32_t _imgWriteMask = WdPipeline::ShaderParameterType::WD_STORAGE_IMAGE & WdPipeline::ShaderParameterType::WD_STAGE_OUTPUT;

            static const uint32_t _bufReadMask = WdPipeline::ShaderParameterType::WD_SAMPLED_BUFFER & WdPipeline::ShaderParameterType::WD_BUFFER_IMAGE & WdPipeline::ShaderParameterType::WD_UNIFORM_BUFFER & WdPipeline::ShaderParameterType::WD_STORAGE_BUFFER;
            static const uint32_t _bufWriteMask = WdPipeline::ShaderParameterType::WD_BUFFER_IMAGE & WdPipeline::ShaderParameterType::WD_STORAGE_BUFFER;

            static const uint32_t _bufferMask = WdPipeline::ShaderParameterType::WD_SAMPLED_BUFFER & WdPipeline::ShaderParameterType::WD_BUFFER_IMAGE & WdPipeline::ShaderParameterType::WD_UNIFORM_BUFFER & WdPipeline::ShaderParameterType::WD_STORAGE_BUFFER;
            static const uint32_t _imageMask = WdPipeline::ShaderParameterType::WD_SAMPLED_IMAGE & WdPipeline::ShaderParameterType::WD_TEXTURE_IMAGE & WdPipeline::ShaderParameterType::WD_STORAGE_IMAGE & WdPipeline::ShaderParameterType::WD_SUBPASS_INPUT & WdPipeline::ShaderParameterType::WD_STAGE_OUTPUT;

            static const VkAccessFlags paramTypeToAccess[] = {
                VK_ACCESS_SHADER_READ_BIT, 
                VK_ACCESS_SHADER_READ_BIT,
                VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT,
                VK_ACCESS_SHADER_READ_BIT,
                VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT,
                VK_ACCESS_UNIFORM_READ_BIT,
                VK_ACCESS_INPUT_ATTACHMENT_READ_BIT,
                VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT,
                VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
            };

            static const VkDescriptorType WdParamTypeToVkDescriptorType[] = {
                VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE,
                VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
                VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER,
                VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER,
                VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT,
                VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
                VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, // this is temp only 
                VK_DESCRIPTOR_TYPE_SAMPLER,
            };

            static VkShaderStageFlagBits WdStagesToVkStages(const WdStageMask stages) const;

            static VkPipelineStageFlags ResInfoToVkStage(const ResourceInfo& resInfo) const;

            static void addDependencies(std::vector<VkSubpassDependency>& dependencies, std::vector<ResourceInfo>& resInfos, uint32_t readMask, uint32_t writeMask);

            template <class T>
            static void updateAttachements(const T& resourceMap, AttachmentMap& attachments, uint8_t& attachmentIndex, std::vector<VkAttachmentReference>& subpassRefs, std::vector<VkImageView>& framebuffer, VkImageLayout layout);

            static void updateUniformResources(const WdPipeline::Uniforms& resourceMap, ImageResources& imageResources, BufferResources& bufferResources, uint8_t pipelineIndex);
            
            template <class T>
            static void updateAttachmentResources(const T& resourceMap, ImageResources& imageResources, uint8_t pipelineIndex);
            
            VkDescriptorSetLayout createDescriptorSetLayout(const WdPipeline::Uniforms& uniforms, const WdPipeline::SubpassInputs& subpassInputs);
            
            static VertexInputDesc createVertexAttributeDescriptionsAndBinding(const WdPipeline::VertexInputs& vertexInputs) const;

            static void addDescriptorPoolSizes(const WdPipeline& pipeline, std::vector<VkDescriptorPoolSize>& poolSizes) const;

            void writeDescriptorSets(const std::vector<VkDescriptorSet>& descriptorSets, const WdPipeline::Uniforms& uniforms, const WdPipeline::SubpassInputs& subpassInputs) const;

            void createDescriptorPool();

            VulkanRenderPass(const std::vector<WdPipeline>& pipelines, VkDevice device);
            void init() override;
            VulkanPipeline createVulkanPipeline(WdPipeline pipeline, uint8_t index);

            VkDevice _device;
            
            VkRenderPass _renderPass;
            Framebuffer _framebuffer; // TODO: need to actually create the framebuffer object
            RenderArea _renderArea;
            VkDescriptorPool _descriptorPool;
            
            const std::vector<WdPipeline>& _pipelines
            std::vector<VulkanPipeline> _vkPipelines;
            
    };

    class VulkanCommandList : public WdCommandList {
        public:
            friend class GraphicsLayer;
            friend class Vulkan::VulkanLayer;

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
            VulkanCommandList();

            VkClearValue WdClearValueToVkClearValue(WdClearValue clearValue, bool isDepthStencil) const;

            VkCommandBuffer _graphicsCommandBuffer; 

            const VulkanRenderPass& _vkRenderPass;
            std::vector<VulkanRenderPass::VulkanPipeline>::iterator _currentPipeline;
    }
}

#endif