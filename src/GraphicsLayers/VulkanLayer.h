#ifndef WADO_VULKAN_LAYER
#define WADO_WD_VULKAN_LAYER

#define GLFW_INCLUDE_VULKAN
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_ENABLE_EXPERIMENTAL
#include <GLFW/glfw3.h>


#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/hash.hpp>

#include "Shader.h"
#include "WdLayer.h"

#include "VulkanCommandList.h"

#include <optional>

// TODO: Swapchain recreation & cleanup functions 

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
        VK_FORMAT_D32_SFLOAT,
        VK_FORMAT_D32_SFLOAT_S8_UINT,
        VK_FORMAT_D24_UNORM_S8_UINT,
    };

    class VulkanLayer : public WdLayer {
        public:
            // Returns ref to a WdImage that represents the screen, can only be used as a fragment output!
            Memory::WdClonePtr<WdImage> getDisplayTarget() override;

            Memory::WdClonePtr<WdImage> create2DImage(WdExtent2D extent, uint32_t mipLevels, WdSampleCount sampleCount, WdFormat imageFormat, WdImageUsageFlags usageFlags, bool multiFrame = false) override;
            
            WdFormat findSupportedHardwareFormat(const std::vector<WdFormat>& formatCandidates, WdImageTiling tiling, WdFormatFeatureFlags features) override;

            Memory::WdClonePtr<WdBuffer> createBuffer(WdSize size, WdBufferUsageFlags usageFlags, bool multiFrame = false) override;
            
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
            // TODO: should this be abstract too?
            static Memory::WdClonePtr<WdLayer> getVulkanLayer();
            static uint32_t getCurrentFrameIndex();
        private:
            VulkanLayer(GLFWwindow* window, bool debugEnabled);

            void init();
            
            static Memory::WdMainPtr<VulkanLayer> _layer;
            static uint32_t currentFrameIndex;

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

            static std::vector<const char*> _validationLayers;

            static std::vector<const char*> _deviceExtensions;

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
            GLFWwindow* _window; // TODO: this should probably be abstracted somehow 
            VkDebugUtilsMessengerEXT _debugMessenger;
            VkSurfaceKHR _surface;

            uint32_t findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties);
            void getSupportedValidationLayers();
            void getSupportedExtensions();
            std::vector<const char *> getRequiredExtensions();
            bool checkStringSubset(const char** superSet, uint32_t superSetCount, const char** subSet, uint32_t subSetCount);
            bool checkRequiredExtensions(std::vector<const char*>& requiredExtensions);
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
            VkPhysicalDeviceProperties _deviceProperties;
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
            Memory::WdMainPtr<WdImage> _swapchainImage;

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
            std::vector<Memory::WdMainPtr<WdImage>> _liveImages;
            std::vector<Memory::WdMainPtr<WdBuffer>> _liveBuffers;
            std::vector<VkSampler> _liveSamplers;
            std::vector<VkFence> _liveFences;
            std::vector<VkSemaphore> _liveSemaphores;
            std::vector<VkCommandPool> _liveCommandPools;
            std::vector<Memory::WdMainPtr<WdPipeline>> _livePipelines;
            std::vector<Memory::WdMainPtr<WdRenderPass>> _liveRenderPasses;
            std::vector<Memory::WdMainPtr<WdCommandList>> _liveCommandLists;

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
            VkSamplerMipmapMode WdFilterToVkSamplerMipMapMode(WdFilterMode mipMapFilter) const;

            VkMemoryPropertyFlags WdImageUsageToVkMemoryFlags(WdImageUsageFlags usageFlags) const;
            VkMemoryPropertyFlags WdBufferUsageToVkMemoryFlags(WdBufferUsageFlags usageFlags) const;

            VkCommandBuffer beginSingleTimeCommands(VkCommandPool commandPool) const;
            void endSingleTimeCommands(VkCommandBuffer commandBuffer, VkCommandPool commandPool, VkQueue queue) const;
    };
};

#endif