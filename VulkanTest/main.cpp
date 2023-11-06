#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <iostream>
#include <stdexcept>
#include <cstdlib>
#include <vector>
#include <cstring>

const uint32_t WIDTH = 800;
const uint32_t HEIGHT = 600;

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

    void getSupportedExtensions() {
        vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr);
        extensions = std::vector<VkExtensionProperties>(extensionCount);
        vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, extensions.data());

        std::cout << "Available extensions:\n";

        for (const VkExtensionProperties& extension : extensions) {
            std::cout << '\t' << extension.extensionName << '\n';
        }
    }

    // Should change signature here to use STL 
    void checkRequiredExtensions(const char** requiredExtensionNames, uint32_t requiredExtensionsCount) {
        std::cout << "Checking for unsupported extensions:\n";
        bool bExistsUnsupportedExtension = false;
        for (uint32_t i = 0; i < requiredExtensionsCount; i++) {
            const char* extensionName = *(requiredExtensionNames + i);
            bool bFoundExtension = false;
            std::cout << '\t' << extensionName;
            for (const VkExtensionProperties& extension : extensions) {
                bFoundExtension |= strcmp(extension.extensionName, extensionName) == 0;
            }
            if (!bFoundExtension) {
                std::cout << " NOT";
            }
            std ::  cout << " found!\n";
            // Just for clarity 
            bExistsUnsupportedExtension |= !bFoundExtension;
        }

        if (bExistsUnsupportedExtension) {
            throw std::runtime_error("Unsupported extensions found");
        }
    }

    void createInstance() {
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

        getSupportedExtensions();

        uint32_t glfwExtensionCount = 0;
        const char** glfwExtensions;
        glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

        checkRequiredExtensions(glfwExtensions, glfwExtensionCount);

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