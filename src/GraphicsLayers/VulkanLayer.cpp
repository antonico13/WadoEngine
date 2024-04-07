#include "GraphicsAbstractionLayer.h"
#include "VulkanLayer.h"

namespace Wado::GAL::Vulkan {

    // containers for garbage collection
    VulkanLayer::liveImages;
    VulkanLayer::liveBuffers;
    VulkanLayer::liveFences;
    VulkanLayer::liveSemaphores;
    VulkanLayer::liveCommandPools;
    VulkanLayer::livePipelines;
    VulkanLayer::liveRenderPasses;

    WdImage VulkanLayer::create2DImage(WdExtent2D extent, uint32_t mipLevels, WdSampleCount sampleCount, WdFormat imageFormat, WdImageUsageFlags usageFlags) {

    }
}