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

#include <memory>

namespace Wado::GAL::Vulkan {

    using SPIRVShaderByteCode = struct SPIRVShaderByteCode {
        const uint32_t* spirvWords;
        size_t wordCount;
    };

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

            // Immediate functions
            void copyBufferToImage(WdBuffer& buffer, WdImage& image, WdExtent2D extent) override;
            void copyBuffer(WdBuffer& srcBuffer, WdBuffer& dstBuffer, WdSize size) override;

            WdFenceHandle createFence(bool signaled = true) override;
            void waitForFences(const std::vector<WdFenceHandle>& fences, bool waitAll = true, uint64_t timeout = UINT64_MAX) override;
            void resetFences(const std::vector<WdFenceHandle>& fences) override;
            
            WdPipeline createPipeline(Shader::Shader vertexShader, Shader::Shader fragmentShader, WdVertexBuilder* vertexBuilder, WdViewportProperties viewportProperties) override;
            WdRenderPass createRenderPass(const std::vector<WdPipeline>& pipelines) override;

            std::unique_ptr<WdCommandList> createCommandList();

            void executeCommandList(const WdCommandList& commandList) override;

            void displayCurrentTarget() override;

            void prepareImageFor(const WdImage& image, WdImageUsage currentUsage, WdImageUsage nextUsage) override;

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

            std::vector<WdImage*> _swapchainWdImages;

            std::vector<VkImageView> _swapchainImageViews;
            std::vector<VkSemaphore> _imageAvailableSemaphores;
            std::vector<VkSemaphore> _renderFinishedSemaphores;

            uint32_t _currentSwapchainImageIndex;

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

            VkCommandBuffer beginSingleTimeCommands(VkCommandPool commandPool) const;
            void endSingleTimeCommands(VkCommandBuffer commandBuffer, VkCommandPool commandPool, VkQueue queue) const;

    };
};

#endif