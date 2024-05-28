#include "VulkanLayer.h"

#include <map>
#include <set>
#include <algorithm>
#include <string>
#include <iostream>

#define MIN(A, B) ((A) < (B) ? (A) : (B))
#define MAX(A, B) ((A) < (B) ? (B) : (A))
#define TO_VK_BOOL(A) ((A) ? (VK_TRUE) : (VK_FALSE))

namespace Wado::GAL::Vulkan {
    
    bool QueueFamilyIndices::isComplete() {
        return graphicsFamily.has_value() && presentFamily.has_value() && transferFamily.has_value();// && computeFamily.has_value();
    };

// UTILS:
    VkMemoryPropertyFlags VulkanLayer::WdImageUsageToVkMemoryFlags(WdImageUsageFlags usageFlags) const {
        // TODO: this might need to change in the future, but for now this works I think
        return VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
    };

    VkMemoryPropertyFlags VulkanLayer::WdBufferUsageToVkMemoryFlags(WdBufferUsageFlags usageFlags) const {
        // TODO: this needs more in-depth analysis
        if (usageFlags & WdBufferUsage::WD_TRANSFER_SRC || usageFlags & WdBufferUsage::WD_UNIFORM_BUFFER) {
            return VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
        };
        return VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
    };


    uint32_t VulkanLayer::findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties) {
        VkPhysicalDeviceMemoryProperties memProperties;
        vkGetPhysicalDeviceMemoryProperties(_physicalDevice, &memProperties);

        for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
            // If the memory is a type that we allow, and it has all the properties
            // we want, get it.
            if (typeFilter & (1 << i) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties) {
                return i;
            }
        }

        throw std::runtime_error("Failed to find suitable memory type!");
    };

    // all of the below are designed to be 1-to-1 with Vulkan 
    VkSampleCountFlagBits VulkanLayer::WdSampleBitsToVkSampleBits(WdSampleCount sampleCount) const {
        return static_cast<VkSampleCountFlagBits>(sampleCount);
    };

    VkImageUsageFlags VulkanLayer::WdImageUsageToVkImageUsage(WdImageUsageFlags imageUsage) const {
        return imageUsage & (~WdImageUsage::WD_PRESENT); //  need to remove the present flag before converting to Vulkan
    };

    VkBufferUsageFlags VulkanLayer::WdBufferUsageToVkBufferUsage(WdBufferUsageFlags bufferUsage) const {
        return bufferUsage;
    };

    // These are used to determine the queue families needed and aspect masks 
    std::vector<uint32_t> VulkanLayer::getImageQueueFamilies(VkImageUsageFlags usage) const {
        std::vector<uint32_t> queueFamilyIndices;
        if (transferUsage & usage) {
            queueFamilyIndices.push_back(_queueIndices.transferFamily.value());
        }

        if (graphicsUsage & usage) {
            queueFamilyIndices.push_back(_queueIndices.graphicsFamily.value());
        }
        return queueFamilyIndices;
    };

    std::vector<uint32_t> VulkanLayer::getBufferQueueFamilies(VkBufferUsageFlags usage) const {
        std::vector<uint32_t> queueFamilyIndices;
        if (bufferTransferUsage & usage) {
            queueFamilyIndices.push_back(_queueIndices.transferFamily.value());
        }

        if (bufferGraphicsUsage & usage) {
            queueFamilyIndices.push_back(_queueIndices.graphicsFamily.value());
        }
        return queueFamilyIndices;
    };

    // TODO:: Where is this used?
    VkImageAspectFlags VulkanLayer::getImageAspectFlags(VkImageUsageFlags usage) const {
        VkImageAspectFlags flags = 0;

        if (usage & VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT) {
            flags |= VK_IMAGE_ASPECT_DEPTH_BIT;
        }

        if (usage & (~VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT)) {
            flags |= VK_IMAGE_ASPECT_COLOR_BIT;
        }

        return flags;
    };

    // 1-to-1 with Vulkan right now
    VkFilter VulkanLayer::WdFilterToVkFilter(WdFilterMode filter) const {
        return static_cast<VkFilter>(filter);
    };

    VkSamplerAddressMode VulkanLayer::WdAddressModeToVkAddressMode(WdAddressMode addressMode) const {
        return static_cast<VkSamplerAddressMode>(addressMode);
    };

    VkSamplerMipmapMode VulkanLayer::WdFilterToVkSamplerMipMapMode(WdFilterMode mipMapFilter) const {
        return static_cast<VkSamplerMipmapMode>(mipMapFilter);
    };

    void VulkanLayer::getSupportedValidationLayers() {
        vkEnumerateInstanceLayerProperties(&_layerCount, nullptr);

        _layers = std::vector<VkLayerProperties>(_layerCount);
        vkEnumerateInstanceLayerProperties(&_layerCount, _layers.data());

        #ifdef NDEBUG
        #else
            std::cout << "Available layers:\n";

            for (const VkLayerProperties& layer : _layers) {
                std::cout << '\t' << layer.layerName << '\n';
            }
        #endif
    };

    void VulkanLayer::getSupportedExtensions() {
        vkEnumerateInstanceExtensionProperties(nullptr, &_extensionCount, nullptr);

        _extensions = std::vector<VkExtensionProperties>(_extensionCount);
        vkEnumerateInstanceExtensionProperties(nullptr, &_extensionCount, _extensions.data());

        #ifdef NDEBUG
        #else
            std::cout << "Available extensions:\n";

            for (const VkExtensionProperties& extension : _extensions) {
                std::cout << '\t' << extension.extensionName << '\n';
            }
        #endif
    };

    std::vector<const char *> VulkanLayer::getRequiredExtensions() {
        uint32_t glfwExtensionCount = 0;
        const char** glfwExtensions;
        glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);
        std::vector<const char *> requiredExtensions(glfwExtensions, glfwExtensions + glfwExtensionCount);

        if (_enableValidationLayers) {
            requiredExtensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
        }

        requiredExtensions.push_back(VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME);
        
        return requiredExtensions;
    };

    bool VulkanLayer::checkStringSubset(const char** superSet, uint32_t superSetCount, const char** subSet, uint32_t subSetCount) {
        bool bFoundAll = true;
        for (int i = 0; i < subSetCount; i++) {
            const char* subElement = *(subSet + i);
            bool bElementFound = false;

            for (int j = 0; j < superSetCount; j++) {
                const char* superElement = *(superSet + j);

                if (strcmp(subElement, superElement) == 0) {
                    bElementFound = true;
                    break;
                }
            }

            #ifdef NDEBUG 
                if (!bElementFound) {
                    std::cout << "\tNot found: " << subElement << '\n'; 
                }
            #else
            #endif
            
            bFoundAll &= bElementFound;
        };

        return bFoundAll;
    };

    bool VulkanLayer::checkRequiredExtensions(std::vector<const char*>& requiredExtensions) {
        getSupportedExtensions();
        std::vector<const char*> supportedExtensionNames(_extensionCount);

        for (int i = 0; i < _extensionCount; i++) {
            supportedExtensionNames[i] = _extensions[i].extensionName;
        };

        return checkStringSubset(supportedExtensionNames.data(), _extensionCount, requiredExtensions.data(), requiredExtensions.size());
    };

    bool VulkanLayer::checkRequiredLayers() {
        getSupportedValidationLayers();
        std::vector<const char*> supportedLayerNames(_layerCount);

        for (int i = 0; i < _layerCount; i++) {
            supportedLayerNames[i] = _layers[i].layerName;
        };

        return checkStringSubset(supportedLayerNames.data(), _layerCount, _validationLayers.data(), _validationLayers.size());
    };

    std::vector<const char*> VulkanLayer::_validationLayers = {
        "VK_LAYER_KHRONOS_validation",
        "VK_LAYER_LUNARG_monitor",
    };

    std::vector<const char*> VulkanLayer::_deviceExtensions = {
        VK_KHR_SWAPCHAIN_EXTENSION_NAME/*,
        VK_NV_DEVICE_DIAGNOSTIC_CHECKPOINTS_EXTENSION_NAME,*/
    };

// END OF UTILS

// Init functions 

    void VulkanLayer::init() {
        createInstance();
        setupDebugMessenger();
        createSurface();
        pickPhysicalDevice();
        createLogicalDevice();
        createGraphicsCommandPool();
        createTransferCommandPool();
        createSwapchain();
    };

    void VulkanLayer::createInstance() {
        if (_enableValidationLayers && !checkRequiredLayers()) {
            throw std::runtime_error("Vulkan validation layers requested, but not available.");
        };

        // Need to make this library agnostic with a windowing library translator
        // eg, in the future get this running on closed platforms as well. 
        // decouple even API from windowing?
        VkApplicationInfo appInfo{};
        appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
        appInfo.pApplicationName = "Hello Triangle"; // TODO: replace wiht application name
        appInfo.applicationVersion = VK_MAKE_VERSION(0, 1, 0);
        appInfo.pEngineName = "Wado Engine"; // TODO: make this a global const along with version and app name, etc 
        appInfo.engineVersion = VK_MAKE_VERSION(0, 1, 0);
        appInfo.apiVersion = VK_API_VERSION_1_3;

        VkInstanceCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
        createInfo.pApplicationInfo = &appInfo;

        VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo{};

        if (_enableValidationLayers) {
            createInfo.enabledLayerCount = static_cast<uint32_t>(_validationLayers.size());
            createInfo.ppEnabledLayerNames = _validationLayers.data();

            populateDebugMessengerCreateInfo(debugCreateInfo);
            // Debug broken on dzn
            createInfo.pNext = (VkDebugUtilsMessengerCreateInfoEXT*) &debugCreateInfo;
        } else {
            createInfo.enabledLayerCount = 0;
            createInfo.pNext = nullptr;
        };

        std::vector<const char*> requiredExtensions = getRequiredExtensions();

        if(!checkRequiredExtensions(requiredExtensions)) {
            throw std::runtime_error("Could not find required extensions for Vulkan");
        };

        createInfo.enabledExtensionCount = static_cast<uint32_t>(requiredExtensions.size());
        createInfo.ppEnabledExtensionNames = requiredExtensions.data();
        createInfo.enabledLayerCount = 0;

        if (vkCreateInstance(&createInfo, nullptr, &_instance) != VK_SUCCESS) {
            throw std::runtime_error("Failed to create Vulkan instance!");
        };

    };

    // Debug Messenger utils

    static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
        VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
        VkDebugUtilsMessageTypeFlagsEXT messageType,
        const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
        void* pUserData) {

            std::cerr << "Validation layer: " << pCallbackData->pMessage << std::endl;

            return VK_FALSE;
    };

    static VkResult CreateDebugUitlsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo, 
        const VkAllocationCallbacks* pAllocator, VkDebugUtilsMessengerEXT* pDebugMessenger) {
            auto func = (PFN_vkCreateDebugUtilsMessengerEXT) vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
            if (func != nullptr) {
                return func(instance, pCreateInfo, pAllocator, pDebugMessenger);
            } else {
                return VK_ERROR_EXTENSION_NOT_PRESENT;
            };
    };

    static void DestroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger, 
        const VkAllocationCallbacks* pAllocator) {
            auto func = (PFN_vkDestroyDebugUtilsMessengerEXT) vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
            if (func != nullptr) {
                func(instance, debugMessenger, pAllocator);
            };
    };

    void VulkanLayer::populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo) {
        createInfo = {};
        createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
        createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
                                     VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
                                     VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;

        createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | 
                                 VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
                                 VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
        createInfo.pfnUserCallback = debugCallback;
        createInfo.pUserData = nullptr;
    };
    
    void VulkanLayer::setupDebugMessenger() {
        if (!_enableValidationLayers) {
            return;
        }

        VkDebugUtilsMessengerCreateInfoEXT createInfo{};
        populateDebugMessengerCreateInfo(createInfo);

        if (CreateDebugUitlsMessengerEXT(_instance, &createInfo, nullptr, &_debugMessenger) != VK_SUCCESS) {
            throw std::runtime_error("Failed to set up Vulkan debug messenger");
        };
    };

    void VulkanLayer::createSurface() {
        if (glfwCreateWindowSurface(_instance, _window, nullptr, &_surface) != VK_SUCCESS) {
            throw std::runtime_error("Failed to create a GLFW Vulkan Window Surface");
        };
    };

    bool VulkanLayer::checkDeviceExtensionSupport(VkPhysicalDevice device) {
        uint32_t extensionCount;
        vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);
        
        std::vector<VkExtensionProperties> availableExtensions(extensionCount);
        vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, availableExtensions.data());
        
        std::vector<const char*> supportedExtensionNames(extensionCount);
        for (int i = 0; i < extensionCount; i++) {
            supportedExtensionNames[i] = availableExtensions[i].extensionName;
        }
        return checkStringSubset(supportedExtensionNames.data(), extensionCount, _deviceExtensions.data(), _deviceExtensions.size());
    };

    VulkanLayer::SwapChainSupportDetails VulkanLayer::querySwapChainSupport(VkPhysicalDevice device) {
        SwapChainSupportDetails details;

        vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, _surface, &details.capabilities);

        uint32_t formatCount;
        vkGetPhysicalDeviceSurfaceFormatsKHR(device, _surface, &formatCount, nullptr);

        if (formatCount != 0) {
            details.formats.resize(formatCount);
            vkGetPhysicalDeviceSurfaceFormatsKHR(device, _surface, &formatCount, details.formats.data());
        };

        uint32_t presentModeCount;
        vkGetPhysicalDeviceSurfacePresentModesKHR(device, _surface, &presentModeCount, nullptr);

        if (presentModeCount != 0) {
            details.presentModes.resize(presentModeCount);
            vkGetPhysicalDeviceSurfacePresentModesKHR(device, _surface, &presentModeCount, details.presentModes.data());
        };

        return details;
    };

    QueueFamilyIndices VulkanLayer::findQueueFamilies(VkPhysicalDevice device) {
        QueueFamilyIndices indices;
        uint32_t queueFamilyCount;
        vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);

        std::vector<VkQueueFamilyProperties> queueFamilyProperties(queueFamilyCount);
        vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilyProperties.data());
        int i = 0;
        for (const VkQueueFamilyProperties& familyProperties : queueFamilyProperties) {
            if (familyProperties.queueFlags & VK_QUEUE_GRAPHICS_BIT) {
                indices.graphicsFamily = i;
                // look specifically for a non-graphics transfer queue.
            } else if (familyProperties.queueFlags & VK_QUEUE_TRANSFER_BIT) {
                indices.transferFamily = i;
            } /*else if (familyProperties.queueFlags & VK_QUEUE_COMPUTE_BIT) {
                indices.computeFamily = i;
            } */

            VkBool32 presentSupport = false;
            vkGetPhysicalDeviceSurfaceSupportKHR(device, i, _surface, &presentSupport);

            if (presentSupport) {
                indices.presentFamily = i;
            }

            if (indices.isComplete()) {
                break;
            }
            i++;
        }
        return indices;
    };

    bool VulkanLayer::isDeviceSuitable(VkPhysicalDevice device) {
        VkPhysicalDeviceProperties deviceProperties;
        vkGetPhysicalDeviceProperties(device, &deviceProperties);
        
        #ifdef NDEBUG
        #else
            std::cout << deviceProperties.deviceName << std::endl;
        #endif

        VkPhysicalDeviceFeatures deviceFeatures;
        vkGetPhysicalDeviceFeatures(device, &deviceFeatures);

        bool extensionsSupported = checkDeviceExtensionSupport(device);

        bool swapChainAdequate = false;
        if (extensionsSupported) {
            SwapChainSupportDetails swapChainSupport = querySwapChainSupport(device);
            swapChainAdequate = !swapChainSupport.formats.empty() && 
                                !swapChainSupport.presentModes.empty();
        };

        QueueFamilyIndices indices = findQueueFamilies(device);
        return indices.isComplete() && extensionsSupported && swapChainAdequate && deviceFeatures.samplerAnisotropy;
    };

    VkSampleCountFlagBits VulkanLayer::getMaxUsableSampleCount() {
        VkPhysicalDeviceProperties physicalDeviceProperties;
        vkGetPhysicalDeviceProperties(_physicalDevice, &physicalDeviceProperties);

        VkSampleCountFlags counts = physicalDeviceProperties.limits.framebufferColorSampleCounts &
                                    physicalDeviceProperties.limits.framebufferDepthSampleCounts;

        if (counts & VK_SAMPLE_COUNT_64_BIT) {
            return VK_SAMPLE_COUNT_64_BIT;
        }
        if (counts & VK_SAMPLE_COUNT_32_BIT) {
            return VK_SAMPLE_COUNT_32_BIT;
        }
        if (counts & VK_SAMPLE_COUNT_16_BIT) {
            return VK_SAMPLE_COUNT_16_BIT;
        }
        if (counts & VK_SAMPLE_COUNT_8_BIT) {
            return VK_SAMPLE_COUNT_8_BIT;
        }
        if (counts & VK_SAMPLE_COUNT_4_BIT) {
            return VK_SAMPLE_COUNT_4_BIT;
        }
        if (counts & VK_SAMPLE_COUNT_2_BIT) {
            return VK_SAMPLE_COUNT_2_BIT;
        }

        return VK_SAMPLE_COUNT_1_BIT;
    };
            
    void VulkanLayer::pickPhysicalDevice() {
        uint32_t deviceCount = 0;
        vkEnumeratePhysicalDevices(_instance, &deviceCount, nullptr);

        if (deviceCount == 0) {
            throw std::runtime_error("Failed to find any GPU with Vulkan support.");
        };

        std::vector<VkPhysicalDevice> devices(deviceCount);
        vkEnumeratePhysicalDevices(_instance, &deviceCount, devices.data());

        for (const VkPhysicalDevice& device: devices) {
            if (isDeviceSuitable(device)) {
                _physicalDevice = device;
                _msaaSamples = getMaxUsableSampleCount();
                _queueIndices = findQueueFamilies(device);
                _swapChainSupportDetails = querySwapChainSupport(device);
                vkGetPhysicalDeviceProperties(_physicalDevice, &_deviceProperties);
                break;
            }; 
        }

        #ifdef NDEBUG
        #else
            VkPhysicalDeviceProperties deviceProperties;
            vkGetPhysicalDeviceProperties(_physicalDevice, &deviceProperties);
            std::cout << "Chosen device " << deviceProperties.deviceName << std::endl;
        #endif

        if (_physicalDevice == VK_NULL_HANDLE) {
            throw std::runtime_error("Could not find a suitable GPU for Vulkan!");
        };
    };
    
    void VulkanLayer::createLogicalDevice() {

        std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
        std::set<uint32_t> uniqueQueueFamilies = {
            _queueIndices.graphicsFamily.value(), 
            _queueIndices.presentFamily.value(),
            _queueIndices.transferFamily.value()
        };

        float queuePriority = 1.0f;

        for (uint32_t queueFamily : uniqueQueueFamilies) {
            VkDeviceQueueCreateInfo queueCreateInfo{};

            queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
            queueCreateInfo.queueFamilyIndex = queueFamily;
            queueCreateInfo.queueCount = 1;
            queueCreateInfo.pQueuePriorities = &queuePriority;
            queueCreateInfos.push_back(queueCreateInfo);
        };

        VkPhysicalDeviceFeatures deviceFeatures{};
        deviceFeatures.samplerAnisotropy = true;
        //deviceFeatures.robustBufferAccess = true; // debug only

        VkDeviceCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
        createInfo.pQueueCreateInfos = queueCreateInfos.data();
        createInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
        createInfo.pEnabledFeatures = &deviceFeatures;

        createInfo.enabledExtensionCount = static_cast<uint32_t>(_deviceExtensions.size());
        createInfo.ppEnabledExtensionNames = _deviceExtensions.data();

        if (_enableValidationLayers) {
            createInfo.enabledLayerCount = static_cast<uint32_t>(_validationLayers.size());
            createInfo.ppEnabledLayerNames = _validationLayers.data();
        } else {
            createInfo.enabledLayerCount = 0;
        };

        if (vkCreateDevice(_physicalDevice, &createInfo, nullptr, &_device) != VK_SUCCESS) {
            throw std::runtime_error("Could not create logical Vulkan device!");
        };

        // TODO: could get multiple queues within a family here? Could help for parallelism. Eg: get all queues of a family 
        vkGetDeviceQueue(_device, _queueIndices.graphicsFamily.value(), 0, &_graphicsQueue);
        vkGetDeviceQueue(_device, _queueIndices.presentFamily.value(), 0, &_presentQueue);
        vkGetDeviceQueue(_device, _queueIndices.transferFamily.value(), 0, &_transferQueue);
        //vkGetDeviceQueue(device, indices.graphicsFamily.value(), 0, &computeQueue);
    };
    
    void VulkanLayer::createGraphicsCommandPool() {
        VkCommandPoolCreateInfo poolInfo{};
        poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
        poolInfo.queueFamilyIndex = _queueIndices.graphicsFamily.value();

        if (vkCreateCommandPool(_device, &poolInfo, nullptr, &_graphicsCommandPool) != VK_SUCCESS) {
            throw std::runtime_error("Failed to create Vulkan graphics command pool");
        };
    };
    
    void VulkanLayer::createTransferCommandPool() {
        VkCommandPoolCreateInfo poolInfo{};
        poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT | VK_COMMAND_POOL_CREATE_TRANSIENT_BIT;
        poolInfo.queueFamilyIndex = _queueIndices.transferFamily.value();

        if (vkCreateCommandPool(_device, &poolInfo, nullptr, &_transferCommandPool) != VK_SUCCESS) {
            throw std::runtime_error("Failed to create Vulkan transfer command pool!");
        };
    };

    VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats) {
        for (const VkSurfaceFormatKHR& availableFormat : availableFormats) {
            if (availableFormat.format == VK_FORMAT_R8G8B8A8_SRGB &&
                availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
                    return availableFormat;
                };
        };
        return availableFormats[0];
    };

    VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes) {
        for (const VkPresentModeKHR& availablePresentMode : availablePresentModes) {
            if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR) {
                return availablePresentMode;
            };
        };
        return VK_PRESENT_MODE_FIFO_KHR;
    };

    VkExtent2D chooseSwapExtent(GLFWwindow* window, const VkSurfaceCapabilitiesKHR& capabilities) {
        if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max()) {
            return capabilities.currentExtent;
        } else {
            int width, height;
            glfwGetFramebufferSize(window, &width, &height);

            VkExtent2D actualExtent = {
                static_cast<uint32_t>(width),
                static_cast<uint32_t>(height)
            };

            actualExtent.width = std::clamp(actualExtent.width, 
                                    capabilities.minImageExtent.width, 
                                    capabilities.maxImageExtent.width);

            actualExtent.width = std::clamp(actualExtent.height, 
                                    capabilities.minImageExtent.height, 
                                    capabilities.maxImageExtent.height);

            return actualExtent;
        };
    };

    void VulkanLayer::createSwapchain() {
        VkSurfaceFormatKHR surfaceFormat = chooseSwapSurfaceFormat(_swapChainSupportDetails.formats);
        VkPresentModeKHR presentMode = chooseSwapPresentMode(_swapChainSupportDetails.presentModes);
        VkExtent2D extent = chooseSwapExtent(_window, _swapChainSupportDetails.capabilities);

        // we want to have frames in flight swap chain images if possible
        uint32_t imageCount = MAX(_swapChainSupportDetails.capabilities.minImageCount + 1, FRAMES_IN_FLIGHT);

        if (_swapChainSupportDetails.capabilities.maxImageCount > 0 && imageCount > _swapChainSupportDetails.capabilities.maxImageCount) {
            imageCount = _swapChainSupportDetails.capabilities.maxImageCount;
        };

        VkSwapchainCreateInfoKHR createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
        createInfo.surface = _surface;

        createInfo.minImageCount = imageCount;
        createInfo.imageFormat = surfaceFormat.format;
        createInfo.imageColorSpace = surfaceFormat.colorSpace;
        createInfo.imageExtent = extent;
        createInfo.imageArrayLayers = 1;
        createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT; // display targets can only be used as fragment outputs (Should I enforce this?)
        // TODO: they could be used for transfer too?
        uint32_t queueFamilyIndices[] = {_queueIndices.graphicsFamily.value(), _queueIndices.presentFamily.value()};

        if (_queueIndices.graphicsFamily != _queueIndices.presentFamily) {
            createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
            createInfo.queueFamilyIndexCount = 2;
            createInfo.pQueueFamilyIndices = queueFamilyIndices;
        } else {
            createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
            createInfo.queueFamilyIndexCount = 0;
            createInfo.pQueueFamilyIndices = nullptr;
        }
        // No transformations, otherwise pull from supportedTransforms
        createInfo.preTransform = _swapChainSupportDetails.capabilities.currentTransform;
        // Blending with other windows in the windowing system, turn off. 
        createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;

        createInfo.presentMode = presentMode;
        // clip obscured pixels from rendering, should be false for full screen
        // space effects in windowed mode? 
        createInfo.clipped = VK_TRUE;
        // whens swapping swapchains due to changes in program execution
        // ie settings or window size or whatever 
        createInfo.oldSwapchain = VK_NULL_HANDLE;

        if (vkCreateSwapchainKHR(_device, &createInfo, nullptr, &_swapchain) != VK_SUCCESS) {
            throw std::runtime_error("Failed to create Vulkan display targets");
        }
        // Get handles for all swap chain images to use in the application 
        vkGetSwapchainImagesKHR(_device, _swapchain, &imageCount, nullptr);
        _swapchainImages.resize(imageCount);
        vkGetSwapchainImagesKHR(_device, _swapchain, &imageCount, _swapchainImages.data());

        _swapchainImageFormat = surfaceFormat.format;
        _swapchainImageExtent = extent;

        _swapchainImageViews.resize(imageCount);

        WdClearValue clearValue;
        clearValue.color = DEFAULT_COLOR_CLEAR;
        
        for (size_t i = 0; i < imageCount; i++) {
            // bad repetition but needed rn 
            VkImageViewCreateInfo viewInfo{};
            viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
            viewInfo.image = _swapchainImages[i];
            viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
            viewInfo.format = _swapchainImageFormat;
            viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            viewInfo.subresourceRange.baseMipLevel = 0;
            viewInfo.subresourceRange.levelCount = 1;
            viewInfo.subresourceRange.baseArrayLayer = 0;
            viewInfo.subresourceRange.layerCount = 1;
            //viewInfo.components to be explicit.
        
            viewInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
            viewInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
            viewInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
            viewInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
       
            if (vkCreateImageView(_device, &viewInfo, nullptr, &_swapchainImageViews[i]) != VK_SUCCESS) {
                throw std::runtime_error("Failed to create Vulkan display image render target!");
            };
        };

        // TODO: deal with multiple frames in flight problem here too
        WdImage* img = create2DImagePtr(reinterpret_cast<const std::vector<WdImageHandle>&>(_swapchainImages), 
                                   std::vector<WdMemoryHandle>(imageCount),  // empty array for the memories
                                   reinterpret_cast<const std::vector<WdRenderTarget>&>(_swapchainImageViews), 
                                   WdFormat::WD_FORMAT_R8G8B8A8_SRGB, // TODO: Vk to Wd format conversion here 
                                   {_swapchainImageExtent.width, _swapchainImageExtent.height}, // TODO: check ordering to be sure 
                                   WdImageUsage::WD_COLOR_ATTACHMENT,
                                   clearValue);

        _swapchainImage = Memory::WdMainPtr<WdImage>(img);

        // create semaphores for render synch 
        VkSemaphoreCreateInfo semaphoreInfo{};
        semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

        _imageAvailableSemaphores.resize(imageCount);
        _renderFinishedSemaphores.resize(imageCount);

        for (size_t i = 0; i < imageCount; i++) {
            if (vkCreateSemaphore(_device, &semaphoreInfo, nullptr, &_imageAvailableSemaphores[i]) != VK_SUCCESS ||
                vkCreateSemaphore(_device, &semaphoreInfo, nullptr, &_renderFinishedSemaphores[i]) != VK_SUCCESS) {
                throw std::runtime_error("Failed to create Vulkan semaphores for rendering!");
            } 
        };
    };

    // GAL functions:

    Memory::WdClonePtr<WdImage> VulkanLayer::getDisplayTarget() {
        return _swapchainImage.getClonePtr();
    };

    Memory::WdClonePtr<WdImage> VulkanLayer::create2DImage(WdExtent2D extent, uint32_t mipLevels, WdSampleCount sampleCount, WdFormat imageFormat, WdImageUsageFlags usageFlags, bool multiFrame) {
        std::vector<VkImage> images(FRAMES_IN_FLIGHT); // image handle, 1-to-1 translation to GAL 
        std::vector<VkDeviceMemory> imageMemories(FRAMES_IN_FLIGHT); // memory handle, 1-to-1
        std::vector<VkImageView> imageViews(FRAMES_IN_FLIGHT); // view handle, same concept 

        VkImageCreateInfo imageInfo{};
        imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        imageInfo.imageType = VK_IMAGE_TYPE_2D;
        imageInfo.extent = {extent.width, extent.height, 1};
        imageInfo.mipLevels = mipLevels;
        imageInfo.arrayLayers = 1;
        imageInfo.format = WdFormatToVkFormat[imageFormat];
        imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL; // start with optimal tiling
        imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED; // should this be undefined to start with?
        imageInfo.usage = WdImageUsageToVkImageUsage(usageFlags);
        imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

        std::vector<uint32_t> queueFamilyIndices = getImageQueueFamilies(imageInfo.usage);
        
        if (queueFamilyIndices.size() > 1) {
            imageInfo.sharingMode = VK_SHARING_MODE_CONCURRENT;
            imageInfo.queueFamilyIndexCount = static_cast<uint32_t>(queueFamilyIndices.size());
            imageInfo.pQueueFamilyIndices = queueFamilyIndices.data();
        }

        imageInfo.samples = WdSampleBitsToVkSampleBits(sampleCount);
        imageInfo.flags = 0;
        
        VkImageAspectFlags flags = getImageAspectFlags(imageInfo.usage);

        int elemCount = multiFrame ? FRAMES_IN_FLIGHT : 1;

        for (int i = 0; i < elemCount; i++) {

            if (vkCreateImage(_device, &imageInfo, nullptr, &images[i]) != VK_SUCCESS) {
                throw std::runtime_error("Failed to create Vulkan image!");
            };

            VkMemoryRequirements memRequirements;
            vkGetImageMemoryRequirements(_device, images[i], &memRequirements);
        
            VkMemoryAllocateInfo allocInfo{};
            allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
            allocInfo.allocationSize = memRequirements.size;
            allocInfo.memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits, usageFlags);

            if (vkAllocateMemory(_device, &allocInfo, nullptr, &imageMemories[i]) != VK_SUCCESS) {
                throw std::runtime_error("Failed to allocate Vulkan image memory!");
            };

            vkBindImageMemory(_device, images[i], imageMemories[i], 0);

            VkImageViewCreateInfo viewInfo{};
            viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
            viewInfo.image = images[i];
            viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
            viewInfo.format = WdFormatToVkFormat[imageFormat];

            viewInfo.subresourceRange.aspectMask = flags;
            viewInfo.subresourceRange.baseMipLevel = 0;
            viewInfo.subresourceRange.levelCount = mipLevels;
            viewInfo.subresourceRange.baseArrayLayer = 0;
            viewInfo.subresourceRange.layerCount = 1;
            //viewInfo.components to be explicit.
        
            viewInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
            viewInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
            viewInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
            viewInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
       
            if (vkCreateImageView(_device, &viewInfo, nullptr, &imageViews[i]) != VK_SUCCESS) {
                throw std::runtime_error("Failed to create Vulkan 2D image view!");
            };
        };

        if (!multiFrame) {
            for (int i = 1; i < FRAMES_IN_FLIGHT; i++) {
                images[i] = images[0];
                imageMemories[i] = imageMemories[0];
                imageViews[i] = imageViews[0];
            };
        };

        // Create GAL resource now 

        WdClearValue clearValue;

        if (flags & VK_IMAGE_ASPECT_DEPTH_BIT) {
            clearValue.depthStencil = DEFAULT_DEPTH_STENCIL_CLEAR;
        } else {
            clearValue.color = DEFAULT_COLOR_CLEAR;
        };

        WdImage* img = create2DImagePtr(reinterpret_cast<const std::vector<WdImageHandle>&>(images), 
                                   reinterpret_cast<const std::vector<WdMemoryHandle>&>(imageMemories), 
                                   reinterpret_cast<const std::vector<WdRenderTarget>&>(imageViews), 
                                   imageFormat,
                                   {extent.height, extent.width},
                                   usageFlags,
                                   clearValue);
        // error check etc (will do with cutom allocator in own new operator)

        return _liveImages.emplace_back(img).getClonePtr(); // keep track of this image 
    };

    Memory::WdClonePtr<WdBuffer> VulkanLayer::createBuffer(WdSize size, WdBufferUsageFlags usageFlags, bool multiFrame) {
        std::vector<VkBuffer> buffers(FRAMES_IN_FLIGHT); // buffer handles
        std::vector<VkDeviceMemory> bufferMemories(FRAMES_IN_FLIGHT); // memory handles

        VkBufferCreateInfo bufferInfo{};
        bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        bufferInfo.size = size;
        bufferInfo.usage = WdBufferUsageToVkBufferUsage(usageFlags);
        bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

        std::vector<uint32_t> queueFamilyIndices = getBufferQueueFamilies(bufferInfo.usage);
        
        if (queueFamilyIndices.size() > 1) {
            bufferInfo.sharingMode = VK_SHARING_MODE_CONCURRENT;
            bufferInfo.queueFamilyIndexCount = static_cast<uint32_t>(queueFamilyIndices.size());
            bufferInfo.pQueueFamilyIndices = queueFamilyIndices.data();
        };

        bufferInfo.flags = 0;

        int elemCount = multiFrame ? FRAMES_IN_FLIGHT : 1;

        for (int i = 0; i < elemCount; i++) {

            if (vkCreateBuffer(_device, &bufferInfo, nullptr, &buffers[i]) != VK_SUCCESS) {
                throw std::runtime_error("Failed to create Vulkan buffer!");
            };

            VkMemoryRequirements memRequirements;
            vkGetBufferMemoryRequirements(_device, buffers[i], &memRequirements);

            VkMemoryAllocateInfo allocInfo{};
            allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
            allocInfo.allocationSize = memRequirements.size;
            allocInfo.memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits, usageFlags);

            if (vkAllocateMemory(_device, &allocInfo, nullptr, &bufferMemories[i]) != VK_SUCCESS) {
                throw std::runtime_error("Failed to allocate Vulkan buffer memory!");
            };

            vkBindBufferMemory(_device, buffers[i], bufferMemories[i], 0);
        };

        if (!multiFrame) {
            for (int i = 1; i < FRAMES_IN_FLIGHT; i++) {
                buffers[i] = buffers[0];
                bufferMemories[i] = bufferMemories[0];
            };
        };

        // Create GAL resource now 
        WdBuffer* buf = createBufferPtr(reinterpret_cast<const std::vector<WdBufferHandle>&>(buffers), 
                                     reinterpret_cast<const std::vector<WdMemoryHandle>&>(bufferMemories),
                                     size, usageFlags);
        // error check etc (will do with cutom allocator in own new operator)

        return _liveBuffers.emplace_back(buf).getClonePtr(); // keep track of this buffer 
    };

    Memory::WdClonePtr<WdImage> VulkanLayer::createTemp2DImage(bool multiFrame) {
        return _liveTempImages.emplace_back(createTemp2DImagePtr()).getClonePtr();
    };

    Memory::WdClonePtr<WdBuffer> VulkanLayer::createTempBuffer(bool multiFrame) {
        return _liveTempBuffers.emplace_back(createTempBufferPtr()).getClonePtr();
    };


    WdSamplerHandle VulkanLayer::createSampler(const WdTextureAddressMode& addressMode, WdFilterMode minFilter, WdFilterMode magFilter, WdFilterMode mipMapFilter) {
        VkSampler textureSampler;

        VkSamplerCreateInfo samplerInfo{};
        samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
        samplerInfo.magFilter = WdFilterToVkFilter(magFilter);
        samplerInfo.minFilter = WdFilterToVkFilter(minFilter);

        samplerInfo.addressModeU = WdAddressModeToVkAddressMode(addressMode.uMode);
        samplerInfo.addressModeV = WdAddressModeToVkAddressMode(addressMode.vMode);;
        samplerInfo.addressModeW = WdAddressModeToVkAddressMode(addressMode.wMode);;
        
        samplerInfo.anisotropyEnable = _enableAnisotropy ? VK_TRUE : VK_FALSE;
        samplerInfo.maxAnisotropy = MIN(_deviceProperties.limits.maxSamplerAnisotropy, _maxAnisotropy);

        samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
        samplerInfo.unnormalizedCoordinates = VK_FALSE;

        samplerInfo.compareEnable = VK_FALSE;
        samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;

        samplerInfo.mipmapMode = WdFilterToVkSamplerMipMapMode(mipMapFilter);
        samplerInfo.mipLodBias = 0.0f;
        samplerInfo.minLod = 0.0f;
        samplerInfo.maxLod = static_cast<float>(_maxMipLevels);

        if (vkCreateSampler(_device, &samplerInfo, nullptr, &textureSampler) != VK_SUCCESS) {
            throw std::runtime_error("Failed to create texture sampler!");
        }

        _liveSamplers.push_back(textureSampler);

        return static_cast<WdSamplerHandle>(textureSampler);
    };

    void VulkanLayer::updateBuffer(const WdBuffer& buffer, void *data, WdSize offset, WdSize dataSize, int bufferIndex) {
        // TODO: should do some bounds checks and stuff here at some point 
        // TODO: offset problem?
        if (bufferIndex == CURRENT_FRAME_RESOURCE) {
            bufferIndex = currentFrameIndex;
        };
        memcpy(buffer.dataPointers[bufferIndex], data, dataSize);
    };

    void VulkanLayer::openBuffer(WdBuffer& buffer, int bufferIndex) {
        if (bufferIndex == CURRENT_FRAME_RESOURCE) {
            bufferIndex = currentFrameIndex;
        };

        vkMapMemory(_device, static_cast<VkDeviceMemory>(buffer.memories[bufferIndex]), 0, buffer.size, 0, &buffer.dataPointers[bufferIndex]);
    };

    // TODO when shutting down, call close buffer on all live buffers if they are open
    void VulkanLayer::closeBuffer(WdBuffer& buffer, int bufferIndex) {
        if (bufferIndex == CURRENT_FRAME_RESOURCE) {
            bufferIndex = currentFrameIndex;
        };

        vkUnmapMemory(_device, static_cast<VkDeviceMemory>(buffer.memories[bufferIndex])); 
    };

    WdFenceHandle VulkanLayer::createFence(bool signaled) {
        VkFence fence;

        VkFenceCreateInfo fenceInfo{};
        fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
        fenceInfo.flags = signaled ? VK_FENCE_CREATE_SIGNALED_BIT : 0; 

        if (vkCreateFence(_device, &fenceInfo, nullptr, &fence) != VK_SUCCESS) {
            throw std::runtime_error("Failed to create Vulkan fence!");
        }

        _liveFences.push_back(fence);

        return static_cast<WdFenceHandle>(fence); 
    };

    void VulkanLayer::waitForFences(const std::vector<WdFenceHandle>& fences, bool waitAll, uint64_t timeout) {
        vkWaitForFences(_device, static_cast<uint32_t>(fences.size()), reinterpret_cast<const VkFence*>(fences.data()), TO_VK_BOOL(waitAll), timeout);
    };

    void VulkanLayer::resetFences(const std::vector<WdFenceHandle>& fences) {
        vkResetFences(_device, static_cast<uint32_t>(fences.size()), reinterpret_cast<const VkFence*>(fences.data()));
    };

    VkCommandBuffer VulkanLayer::beginSingleTimeCommands(VkCommandPool commandPool) const {
        VkCommandBufferAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        allocInfo.commandPool = commandPool;
        allocInfo.commandBufferCount = 1;

        VkCommandBuffer commandBuffer;
        vkAllocateCommandBuffers(_device, &allocInfo, &commandBuffer);

        VkCommandBufferBeginInfo beginInfo{};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

        vkBeginCommandBuffer(commandBuffer, &beginInfo);

        return commandBuffer;
    };

    void VulkanLayer::endSingleTimeCommands(VkCommandBuffer commandBuffer, VkCommandPool commandPool, VkQueue queue) const {
        vkEndCommandBuffer(commandBuffer);

        VkSubmitInfo submitInfo{};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &commandBuffer;

        //TODO: This is pretty bad, should change 
        vkQueueSubmit(queue, 1, &submitInfo, VK_NULL_HANDLE);
        vkQueueWaitIdle(queue);
        vkFreeCommandBuffers(_device, commandPool, 1, &commandBuffer);
    };

    void VulkanLayer::copyBufferToImage(const WdBuffer& buffer, const WdImage& image, WdExtent2D extent, int resourceIndex) {
        if (resourceIndex == CURRENT_FRAME_RESOURCE) {
            resourceIndex = currentFrameIndex;
        };

        VkCommandBuffer commandBuffer = beginSingleTimeCommands(_transferCommandPool);
        VkBufferImageCopy region{};
        region.bufferOffset = 0;
        region.bufferRowLength = 0;
        region.bufferImageHeight = 0;

        region.imageSubresource.aspectMask = getImageAspectFlags(WdImageUsageToVkImageUsage(image.usage));// VK_IMAGE_ASPECT_COLOR_BIT;//| VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT;
        region.imageSubresource.mipLevel = 0;
        region.imageSubresource.baseArrayLayer = 0;
        region.imageSubresource.layerCount = 1;

        region.imageOffset = {0, 0, 0};
        region.imageExtent = {
            image.extent.width, 
            image.extent.height, 
            1
        };

        vkCmdCopyBufferToImage(commandBuffer, static_cast<VkBuffer>(buffer.handles[resourceIndex]), static_cast<VkImage>(image.handles[resourceIndex]), VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);
        
        //std::cout << "Submitted copy buffer to image command" << std::endl;
        
        endSingleTimeCommands(commandBuffer, _transferCommandPool, _transferQueue);       
    };

    void VulkanLayer::copyBuffer(const WdBuffer& srcBuffer, const WdBuffer& dstBuffer, WdSize size, int bufferIndex) {
        if (bufferIndex == CURRENT_FRAME_RESOURCE) {
            bufferIndex = currentFrameIndex;
        };

        VkCommandBuffer commandBuffer = beginSingleTimeCommands(_transferCommandPool);

        VkBufferCopy copyRegion{};
        copyRegion.srcOffset = 0;
        copyRegion.dstOffset = 0;
        copyRegion.size = size;

        vkCmdCopyBuffer(commandBuffer, static_cast<VkBuffer>(srcBuffer.handles[bufferIndex]), static_cast<VkBuffer>(dstBuffer.handles[bufferIndex]), 1, &copyRegion);

        endSingleTimeCommands(commandBuffer, _transferCommandPool, _transferQueue);          
    };

    Memory::WdClonePtr<WdCommandList> VulkanLayer::createCommandList() {
        // TODO: Not sure if I should allocate one every time
        // TODO: when a command list is released, make sure to deallocate its buffers
        // TODO: Same for descriptor sets for pipelines and render passes 

        std::vector<VkCommandBuffer> graphicsBuffers(FRAMES_IN_FLIGHT);
        std::vector<VkCommandBuffer> transferBuffers(FRAMES_IN_FLIGHT);
        
        VkCommandBufferAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        allocInfo.commandPool = _graphicsCommandPool;
        allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        allocInfo.commandBufferCount = FRAMES_IN_FLIGHT;

        if (vkAllocateCommandBuffers(_device, &allocInfo, graphicsBuffers.data()) != VK_SUCCESS) {
            throw std::runtime_error("Failed to allocate Vulkan Command list graphics buffers!");
        };

        VkCommandBufferAllocateInfo transferAllocInfo{};
        transferAllocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        transferAllocInfo.commandPool = _transferCommandPool;
        transferAllocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        transferAllocInfo.commandBufferCount = FRAMES_IN_FLIGHT;

        if (vkAllocateCommandBuffers(_device, &transferAllocInfo, transferBuffers.data()) != VK_SUCCESS) {
            throw std::runtime_error("Failed to allocate Vulkan Command list transfer buffers!");
        };


        VulkanCommandList* cmdList = new VulkanCommandList(graphicsBuffers, transferBuffers);
        // I think this should be fine 
        return _liveCommandLists.emplace_back(cmdList).getClonePtr();
    };

    void VulkanLayer::executeCommandList(const WdCommandList& commandList, WdFenceHandle fenceToSignal) {
        
        const VulkanCommandList& vkCommandList = dynamic_cast<const VulkanCommandList&>(commandList);

        // Submitting graphics queue only right now 

        VkResult result = vkAcquireNextImageKHR(_device, _swapchain, UINT64_MAX, _imageAvailableSemaphores[currentFrameIndex], VK_NULL_HANDLE, &_currentSwapchainIndex);

        if (result == VK_ERROR_OUT_OF_DATE_KHR) {// || bFramebufferResized) {
            //bFramebufferResized = false;
            //recreateSwapChain();
            throw std::runtime_error("Vulkan display target is out of date!");
            return;
        } else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
            throw std::runtime_error("Failed to acquire swap chain image!");
        };

        VkSubmitInfo submitInfo{};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

        // wait until the image is available 
        VkSemaphore waitSemaphores[] = {_imageAvailableSemaphores[currentFrameIndex]};
        // wait for the color attachment output stage to finish 
        // TODO: This can become more complicated with compute and other synchronisation 
        VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
        submitInfo.waitSemaphoreCount = 1;
        submitInfo.pWaitSemaphores = waitSemaphores;
        submitInfo.pWaitDstStageMask = waitStages;

        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &vkCommandList._graphicsCommandBuffers[currentFrameIndex]; 

        // signal that rendering for this frame is finished 
        VkSemaphore signalSemaphores[] = {_renderFinishedSemaphores[currentFrameIndex]};
        submitInfo.signalSemaphoreCount = 1;
        submitInfo.pSignalSemaphores = signalSemaphores;

        // signal the in flight fence when we are done so the CPU can work with this frame again
        VkResult submitResult = vkQueueSubmit(_graphicsQueue, 1, &submitInfo, static_cast<VkFence>(fenceToSignal));
        if (submitResult != VK_SUCCESS) {
            throw std::runtime_error("Failed to submit Vulkan graphics command list!");
        };
    };

    void VulkanLayer::displayCurrentTarget() {
        VkPresentInfoKHR presentInfo{};
        presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
        presentInfo.waitSemaphoreCount = 1;
        // only present when the rendering is finished 
        presentInfo.pWaitSemaphores = &_renderFinishedSemaphores[currentFrameIndex]; 

        presentInfo.swapchainCount = 1;
        presentInfo.pSwapchains = &_swapchain;
        presentInfo.pImageIndices = &_currentSwapchainIndex;
        presentInfo.pResults = nullptr;

        VkResult result = vkQueuePresentKHR(_presentQueue, &presentInfo);
        // TODO: deal with result here (eg resizing, error)
    };

    // Skeleton for now, needs to be expanded 
    Memory::WdClonePtr<WdPipeline> VulkanLayer::createPipeline(const Shader::WdShader& vertexShader, const Shader::WdShader& fragmentShader, const WdViewportProperties& viewportProperties) {
        SPIRVShaderByteCode vertexShaderCode = {reinterpret_cast<const uint32_t*>(vertexShader.shaderByteCode.data()), static_cast<uint32_t>(vertexShader.shaderByteCode.size())};
        SPIRVShaderByteCode fragmentShaderCode = {reinterpret_cast<const uint32_t*>(fragmentShader.shaderByteCode.data()), static_cast<uint32_t>(fragmentShader.shaderByteCode.size())};
        
        // TODO: this is duplicated here and in VulkanCommandList. Make function?
        VkViewport viewport{};
        viewport.x = static_cast<float>(viewportProperties.startCoords.width);
        viewport.y = static_cast<float>(viewportProperties.startCoords.height);
        viewport.width = static_cast<float>(viewportProperties.endCoords.width);
        viewport.height = static_cast<float>(viewportProperties.endCoords.height);
        viewport.minDepth = viewportProperties.depth.min;
        viewport.maxDepth = viewportProperties.depth.max;

        VkRect2D scissor{};
        scissor.offset = {static_cast<int32_t>(viewportProperties.scissor.offset.width), static_cast<int32_t>(viewportProperties.scissor.offset.height)};
        scissor.extent = {viewportProperties.scissor.extent.width, viewportProperties.scissor.extent.height};

        VulkanPipeline* vulkanPipeline = new VulkanPipeline(vertexShaderCode, fragmentShaderCode, viewport, scissor);
        
        return _livePipelines.emplace_back(vulkanPipeline).getClonePtr();
    };

    // creates render pass and all subpasses 
    Memory::WdClonePtr<WdRenderPass> VulkanLayer::createRenderPass(const std::vector<WdPipeline>& pipelines) {
        // TODO: Should assert here and for similar casts that I can do this 
        VulkanRenderPass *vkRenderPass = new VulkanRenderPass(reinterpret_cast<const std::vector<VulkanPipeline>&>(pipelines), _device, {0, 0}, _swapchainImageExtent);
        vkRenderPass->init();
        return _liveRenderPasses.emplace_back(vkRenderPass).getClonePtr();
    };

    //void VulkanRenderPass::prepareImageFor(const WdImage& image, WdImageUsage currentUsage, WdImageUsage nextUsage) {
        // TODO
        /*VkCommandBuffer commandBuffer = beginSingleTimeCommands(_graphicsCommandPool);

        VkImageMemoryBarrier barrier{};
        barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        barrier.oldLayout = oldLayout;
        barrier.newLayout = newLayout;
        barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.image = image;
        if (newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL) {
            barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
            if (hasStencilComponent(format)) {
                barrier.subresourceRange.aspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;
            }
        } else {
            barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT; 
        }
        barrier.subresourceRange.baseMipLevel = 0;
        barrier.subresourceRange.levelCount = mipLevels;
        barrier.subresourceRange.baseArrayLayer = 0;
        barrier.subresourceRange.layerCount = 1;
        //barrier.srcAccessMask = 0; // TODO
        //barrier.dstAccessMask = 0; // TOOD

        VkPipelineStageFlags sourceStage;
        VkPipelineStageFlags destinationStage;
        
        if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
            barrier.srcAccessMask = 0;
            barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

            sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
            destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
        } else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
            barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
            barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
            
            sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
            destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
        } else if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL) {
            barrier.srcAccessMask = 0;
            barrier.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | 
                                    VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
            
            sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
            destinationStage = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
        } else { 
            throw std::invalid_argument("Unsupported layout transition!");
        }

        vkCmdPipelineBarrier(commandBuffer, sourceStage, destinationStage, 0, 
                            0, nullptr, 
                            0, nullptr, 
                            1, &barrier);

        endSingleTimeCommands(commandBuffer, commandPool, graphicsQueue);        */
    //};

    static const VkImageTiling WdTilingToVkTiling(const WdImageTiling imgTiling) {
        return static_cast<VkImageTiling>(imgTiling);
    };

    static const VkFormatFeatureFlags WdFeatureFlagsToVkFeatureFlags(const WdFormatFeatureFlags features) {
        return features;
    };

    WdFormat VulkanLayer::findSupportedHardwareFormat(const std::vector<WdFormat>& formatCandidates, WdImageTiling tiling, WdFormatFeatureFlags features) {
        for (WdFormat format : formatCandidates) {
            VkFormatProperties properties;
            vkGetPhysicalDeviceFormatProperties(_physicalDevice, WdFormatToVkFormat[format], &properties);
            if (WdTilingToVkTiling(tiling) == VK_IMAGE_TILING_LINEAR && (properties.linearTilingFeatures & WdFeatureFlagsToVkFeatureFlags(features)) == features) {
                return format;
            } else if (WdTilingToVkTiling(tiling) == VK_IMAGE_TILING_OPTIMAL && (properties.optimalTilingFeatures & WdFeatureFlagsToVkFeatureFlags(features)) == features) {
                return format;
            }; 
        };

        throw std::runtime_error("Failed to find a supported format!");        
    };


    // Setup and init 
    VulkanLayer::VulkanLayer(GLFWwindow* window, bool debugEnabled) : _window(window), _enableValidationLayers(debugEnabled) {
        init();
    };

    Memory::WdClonePtr<WdLayer> VulkanLayer::getVulkanLayer() { 

    };

};