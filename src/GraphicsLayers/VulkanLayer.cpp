#include "GraphicsAbstractionLayer.h"
#include "VulkanLayer.h"

#define MIN(A, B) ((A) < (B) ? (A) : (B))

namespace Wado::GAL::Vulkan {
    
    bool QueueFamilyIndices::isComplete() {
        return graphicsFamily.has_value() && presentFamily.has_value() && transferFamily.has_value();// && computeFamily.has_value();
    };

// UTILS:

    // all of the below are designed to be 1-to-1 with Vulkan 
    VkSampleCountFlagBits VulkanLayer::WdSampleBitsToVkSampleBits(WdSampleCount sampleCount) const {
        return sampleCount;
    };

    VkImageUsageFlags VulkanLayer::WdImageUsageToVkImageUsage(WdImageUsageFlags imageUsage) const {
        return imageUsage;
    };

    VkBufferUsageFlags VulkanLayer::WdBufferUsageToVkBufferUsage(WdBufferUsageFlags bufferUsage) const {
        return bufferUsage;
    }

    // These are used to determine the queue families needed and aspect masks 
    std::vector<uint32_t> VulkanLayer::getImageQueueFamilies(VkImageUsageFlags usage) const {
        std::vector<uint32_t> queueFamilyIndices;
        if (transferUsage & usage) {
            queueFamilyIndices.push_back(queueIndices.transferFamily.value());
        }

        if (graphicsUsage & usage) {
            queueFamilyIndices.push_back(queueIndices.graphicsFamily.value());
        }
        return queueFamilyIndices;
    };

    std::vector<uint32_t> getBufferQueueFamilies(VkBufferUsageFlags usage) const {
        std::vector<uint32_t> queueFamilyIndices;
        if (bufferTransferUsage & usage) {
            queueFamilyIndices.push_back(queueIndices.transferFamily.value());
        }

        if (bufferGraphicsUsage & usage) {
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

    // 1-to-1 with Vulkan right now
    VkFilter VulkanLayer::WdFilterToVkFilter(WdFilterMode filter) const {
        return filter;
    };

    VkSamplerAddressMode VulkanLayer::WdAddressModeToVkAddressMode(WdAddressMode addressMode) const {
        return addressMode;
    };



// END OF UTILS

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

        return *img;
    };

    WdBuffer VulkanLayer::createBuffer(WdSize size, WdBufferUsageFlags usageFlags) {
        VkBuffer buffer; // buffer handle 
        VkMemory bufferMemory; // memory handle 

        VkBufferCreateInfo bufferInfo{};
        bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        bufferInfo.size = size;
        bufferInfo.usage = WdBufferUsageToVkBufferUsage(usage);
        bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

        std::vector<uint32_t> queueFamilyIndices = getBufferQueueFamilies(bufferInfo.usage);
        
        if (queueFamilyIndices.size() > 1) {
            bufferInfo.sharingMode = VK_SHARING_MODE_CONCURRENT;
            bufferInfo.queueFamilyIndexCount = static_cast<uint32_t>(queueFamilyIndices.size());
            bufferInfo.pQueueFamilyIndices = queueFamilyIndices.data();
        }

        bufferInfo.flags = 0;

        if (vkCreateBuffer(device, &bufferInfo, nullptr, &buffer) != VK_SUCCESS) {
            throw std::runtime_error("Failed to create buffer!");
        }

        VkMemoryRequirements memRequirements;
        vkGetBufferMemoryRequirements(device, buffer, &memRequirements);

        VkMemoryAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        allocInfo.allocationSize = memRequirements.size;
        allocInfo.memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits, properties);

        if (vkAllocateMemory(device, &allocInfo, nullptr, &bufferMemory) != VK_SUCCESS) {
            throw std::runtime_error("Failed to allocate buffer memory!");
        }

        vkBindBufferMemory(device, buffer, bufferMemory, 0);

        // Create GAL resource now 
        WdBuffer* buf = new WdBuffer;
        // error check etc (will do with cutom allocator in own new operator)

        buf->handle = static_cast<WdBufferHandle>(buffer);
        buf->memory = static_cast<WdMemoryPointer>(bufferMemory); 
        buf->size = size;

        liveBuffers.push_back(buf); // keep track of this image 

        return *buf;
    };

    WdSamplerHandle VulkanLayer::createSampler(WdTextureAddressMode addressMode, WdFilterMode minFilter, WdFilterMode magFilter, WdFilterMode mipMapFilter) {
        VkSampler textureSampler;

        VkSamplerCreateInfo samplerInfo{};
        samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
        samplerInfo.magFilter = WdFilterToVkFilter(magFilter);
        samplerInfo.minFilter = WdFilterToVkFilter(minFilter);

        samplerInfo.addressModeU = WdAddressModeToVkAddressMode(addressMode.uMode);
        samplerInfo.addressModeV = WdAddressModeToVkAddressMode(addressMode.vMode);;
        samplerInfo.addressModeW = WdAddressModeToVkAddressMode(addressMode.wMode);;
        
        VkPhysicalDeviceProperties properties{};
        vkGetPhysicalDeviceProperties(physicalDevice, &properties);

        samplerInfo.anisotropyEnable = enableAnisotropy ? VK_TRUE : VK_FALSE;
        samplerInfo.maxAnisotropy = MIN(properties.limits.maxSamplerAnisotropy, maxAnisotropy);

        samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
        samplerInfo.unnormalizedCoordinates = VK_FALSE;

        samplerInfo.compareEnable = VK_FALSE;
        samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;

        samplerInfo.mipmapMode = WdAddressModeToVkAddressMode(mipMapFilter);
        samplerInfo.mipLodBias = 0.0f;
        samplerInfo.minLod = 0.0f;
        samplerInfo.maxLod = static_cast<float>(maxMipLevels);

        if (vkCreateSampler(device, &samplerInfo, nullptr, &textureSampler) != VK_SUCCESS) {
            throw std::runtime_error("Failed to create texture sampler!");
        }

        liveSamplers.push_back(static_cast<WdSamplerHandle>(textureSampler));

        return static_cast<WdSamplerHandle>(textureSampler);
    };

    void VulkanLayer::updateBuffer(WdBuffer buffer, void * data, WdSize offset, WdSize dataSize) {
        // TODO: should do some bounds checks and stuff here at some point 
        memcpy(buffer.data + offset, data, dataSize);
    };

    void VulkanLayer::openBuffer(WdBuffer& buffer) {
        vkMapMemory(device, buffer.memory, 0, buffer.size, 0, &buffer.data);
    };

    // TODO when shutting down, call close buffer on all live buffers if they are open
    void VulkanLayer::closeBuffer(WdBuffer& buffer) {
        vkUnmapMemory(device, buffer.memory); 
    };

}