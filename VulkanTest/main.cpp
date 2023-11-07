#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <iostream>
#include <stdexcept>
#include <cstdlib>
#include <vector>
#include <cstring>

const uint32_t WIDTH = 800;
const uint32_t HEIGHT = 600;

std::vector<const char*> validationLayers = {
    "VK_LAYER_KHRONOS_validation"
};

#ifdef NDEBUG
    const bool bEnableValidationLayers = false;
#else
    const bool bEnableValidationLayers = true;
#endif

class HelloTriangleApplication {
public:
    void run() {
        initWindow();
        initVulkan();
        mainLoop();
        cleanup();
    }

private:
    GLFWwindow* window;
    VkInstance instance;
    uint32_t extensionCount = 0;
    std::vector<VkExtensionProperties> extensions;
    uint32_t layerCount = 0;
    std::vector<VkLayerProperties> layers;

    void getSupportedValidationLayers() {
        vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

        layers = std::vector<VkLayerProperties>(layerCount);
        vkEnumerateInstanceLayerProperties(&layerCount, layers.data());

        #ifdef NDEBUG
        #else
            std::cout << "Available layers:\n";

            for (const VkLayerProperties& layer : layers) {
                std::cout << '\t' << layer.layerName << '\n';
            }
        #endif
    }

    void getSupportedExtensions() {
        vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr);

        extensions = std::vector<VkExtensionProperties>(extensionCount);
        vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, extensions.data());

        #ifdef NDEBUG
        #else
            std::cout << "Available extensions:\n";

            for (const VkExtensionProperties& extension : extensions) {
                std::cout << '\t' << extension.extensionName << '\n';
            }
        #endif
    }

    bool checkStringSubset(const char** superSet, uint32_t superSetCount, const char** subSet, uint32_t subSetCount) {
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
        }
        return bFoundAll;
    }

    bool checkRequiredExtensions(const char** requiredExtensionNames, uint32_t requiredExtensionsCount) {
        getSupportedExtensions();
        std::vector<const char*> supportedExtensionNames(extensionCount);
        for (int i = 0; i < extensionCount; i++) {
            supportedExtensionNames[i] = extensions[i].extensionName;
        }
        return checkStringSubset(supportedExtensionNames.data(), extensionCount, requiredExtensionNames, requiredExtensionsCount);
    }

    bool checkRequiredLayers() {
        getSupportedValidationLayers();
        std::vector<const char*> supportedLayerNames(layerCount);
        for (int i = 0; i < layerCount; i++) {
            supportedLayerNames[i] = layers[i].layerName;
        }
        return checkStringSubset(supportedLayerNames.data(), layerCount, validationLayers.data(), validationLayers.size());
    }

    void createInstance() {
        if (bEnableValidationLayers && !checkRequiredLayers()) {
            throw std::runtime_error("Validation layers requested, but not available.");
        }

        // Need to make this library agnostic with a windowing library translator
        // eg, in the future get this running on closed platforms as well. 
        // decouple even API from windowing?
        VkApplicationInfo appInfo{};
        appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
        appInfo.pApplicationName = "Hello Triangle";
        appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
        appInfo.pEngineName = "No Engine";
        appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
        appInfo.apiVersion = VK_API_VERSION_1_0;

        VkInstanceCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
        createInfo.pApplicationInfo = &appInfo;

        if (bEnableValidationLayers) {
            createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
            createInfo.ppEnabledLayerNames = validationLayers.data();
        } else {
            createInfo.enabledLayerCount = 0;
        }

        uint32_t glfwExtensionCount = 0;
        const char** glfwExtensions;
        glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

        if(!checkRequiredExtensions(glfwExtensions, glfwExtensionCount)) {
            throw std::runtime_error("Could not find required extensions.");
        }

        createInfo.enabledExtensionCount = glfwExtensionCount;
        createInfo.ppEnabledExtensionNames = glfwExtensions;
        createInfo.enabledLayerCount = 0;

        if (vkCreateInstance(&createInfo, nullptr, &instance) != VK_SUCCESS) {
            throw std::runtime_error("Failed to create Vulkan instance");
        };

    }

    void initWindow() {
        glfwInit();
        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
        window = glfwCreateWindow(WIDTH, HEIGHT, "Vulkan", nullptr, nullptr);
    }
    void initVulkan() {
        createInstance();
    }   

    void mainLoop() {
        while ((!glfwWindowShouldClose(window))) {
            glfwPollEvents();
        }
    } 

    void cleanup() {
        vkDestroyInstance(instance, nullptr);

        glfwDestroyWindow(window);

        glfwTerminate();
    }
};

int main() {
    HelloTriangleApplication app;

    try {
        app.run();
    } catch (const std::exception& e) {
        std::cerr << e.what() << std::endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}