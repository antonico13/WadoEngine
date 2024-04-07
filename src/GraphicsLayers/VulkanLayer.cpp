#include "GraphicsAbstractionLayer.h"
#include "VulkanLayer.h"

namespace Wado::GAL::Vulkan {
    
    bool QueueFamilyIndices::isComplete() {
        return graphicsFamily.has_value() && presentFamily.has_value() && transferFamily.has_value();// && computeFamily.has_value();
    };

    VkSampleCountFlagBits VulkanLayer::WdSampleBitsToVkSampleBits(WdSampleCount sampleCount) const {
        return sampleCount;
    };

    VkImageUsageFlags VulkanLayer::WdImageUsageToVkImageUsage(WdImageUsageFlags imageUsage) const {
        return imageUsage;
    };

    std::vector<uint32_t> VulkanLayer::getImageQueueFamilies(VkImageUsageFlags usage) const {
        VkImageUsageFlags imageTransferUsage = transferUsage & imageInfo.usage;
        VkImageUsageFlags imageGraphicsUsage = graphics & imageInfo.usage;
        std::vector<uint32_t> queueFamilyIndices;
        if (transferUsage & imageInfo.usage) {
            queueFamilyIndices.push_back(queueIndices.transferFamily.value());
        }

        if (graphicsUsage & imageInfo.usage) {
            queueFamilyIndices.push_back(queueIndices.graphicsFamily.value());
        }
        return queueFamilyIndices;
    };

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

    WdImage VulkanLayer::create2DImage(WdExtent2D extent, uint32_t mipLevels, WdSampleCount sampleCount, WdFormat imageFormat, WdImageUsageFlags usageFlags) {
        VkImage image; // image handle, 1-to-1 translation to GAL 
        VkDeviceMemory imageMemory; // memory handle, 1-to-1
        VkImageView imageView; // view handle, same concept 

        VkImageCreateInfo imageInfo{};
        imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        imageInfo.imageType = VK_IMAGE_TYPE_2D;
        imageInfo.extent = {extent.width, extent.height, 1};
        imageInfo.mipLevels = mipLevels;
        imageInfo.arrayLayers = 1;
        imageInfo.format = WdFormatToVKFormat[format];
        imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
        imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        imageInfo.usage = WdImageUsageToVkImageUsage(usage);
        imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

        std::vector<uint32_t> queueFamilyIndices = getImageQueueFamilies(imageInfo.usage);
        
        if (queueFamilyIndices.size() > 1) {
            imageInfo.sharingMode = VK_SHARING_MODE_CONCURRENT;
            imageInfo.queueFamilyIndexCount = static_cast<uint32_t>(queueFamilyIndices.size());
            imageInfo.pQueueFamilyIndices = queueFamilyIndices.data();
        }

        imageInfo.samples = WdSampleBitsToVkSampleBits(numSamples);
        imageInfo.flags = 0;

        if (vkCreateImage(device, &imageInfo, nullptr, &image) != VK_SUCCESS) {
            throw std::runtime_error("Failed to create image!");
        }

        VkMemoryRequirements memRequirements;
        vkGetImageMemoryRequirements(device, image, &memRequirements);
        
        VkMemoryAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        allocInfo.allocationSize = memRequirements.size;
        allocInfo.memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits, properties);

        if (vkAllocateMemory(device, &allocInfo, nullptr, &imageMemory) != VK_SUCCESS) {
            throw std::runtime_error("Failed to allocate image memory!");
        }

        vkBindImageMemory(device, image, imageMemory, 0);

        VkImageViewCreateInfo viewInfo{};
        viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        viewInfo.image = image;
        viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        viewInfo.format = WdFormatToVKFormat[format];

        VkImageAspectFlags flags = getImageAspectFlags(imageInfo.usage);

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
       
        if (vkCreateImageView(device, &viewInfo, nullptr, &imageView) != VK_SUCCESS) {
            throw std::runtime_error("Failed to create 2D image view!");
        }

        // Create GAL resource now 
        WdImage* img = new WdImage;
        // error check etc (will do with cutom allocator in own new operator)

        img->handle = static_cast<WdImageHandle>(image);
        img->memory = static_cast<WdMemoryPointer>(imageMemory); 
        img->target = static_cast<WdRenderTarget>(imageView); 
        img->format = format;
        img->extent = {extent.height, extent.width, 1};
        if (flags & VK_IMAGE_ASPECT_DEPTH_BIT) {
            img->clearValue.depthStencil = defaultDepthStencilClear;
        } else {
            img->clearValue.color = defaultColorClear;
        }

        liveImages.push_back(img); // keep track of this image 
    }
}