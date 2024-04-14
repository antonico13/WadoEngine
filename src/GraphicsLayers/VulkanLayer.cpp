#include "GraphicsAbstractionLayer.h"
#include "VulkanLayer.h"

#include <map>
#include <string>


#define MIN(A, B) ((A) < (B) ? (A) : (B))
#define TO_VK_BOOL(A) ((A) ? (VK_TRUE) : (VK_FALSE))

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
        return imageUsage & (~WdImageUsage::WD_PRESENT); //  need to remove the present flag before converting to Vulkan
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
        img->usage = usageFlags;
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


    WdFenceHandle VulkanLayer::createFence(bool signaled) {
        VkFence fence;

        VkFenceCreateInfo fenceInfo{};
        fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
        fenceInfo.flags = signaled ? VK_FENCE_CREATE_SIGNALED_BIT : 0; 

        if (vkCreateFence(device, &fenceInfo, nullptr, &fence) != VK_SUCCESS) {
            throw std::runtime_error("Failed to create semaphores and fence for graphics!");
        }

        liveFences.push_back(fence);

        return static_cast<WdFenceHandle>(fence); 
    };

    void VulkanLayer::waitForFences(std::vector<WdFenceHandle> fences, bool waitAll, uint64_t timeout) {
        // should probably do a static cast here for fence data?
        vkWaitForFences(device, static_cast<uint32_t>(fences.size()), fences.data(), TO_VK_BOOL(waitAll), timeout);
    };

    void VulkanLayer::resetFences(std::vector<WdFenceHandle> fences) {
        vkResetFences(device, static_cast<uint32_t>(fences.size()), fences.data());
    };

    // Skeleton for now, needs to be expanded 
    WdPipeline VulkanLayer::createPipeline(Shader::Shader vertexShader, Shader::Shader fragmentShader, WdVertexBuilder* vertexBuilder, WdViewportProperties viewportProperties) {
        return WdPipeline(vertexShader, fragmentShader, vertexBuilder, viewportProperties);
    };

    // creates render pass and all subpasses 
    WdRenderPass VulkanLayer::createRenderPass(std::vector<WdPipeline> pipelines, std::vector<WdImage> attachments) {
        
    };

    // Render pass creation & utils 

    // need to figure out how to handle samples here, for now everything is 1 sample 
    #define UPDATE_ATTACHMENTS(ATTACH_MAP, ATTACH_LAYOUT, CONTAINER) \
    for (std::map<std::string, WdPipeline::ShaderParameter>::iterator it = ATTACH_MAP.begin(); it != ATTACH_MAP.end(); ++it) { \
        WdImageHandle attachmentHandle = it->second.resource.imageResource.image->handle; \
        std::map<WdImageHandle, AttachmentInfo>::iterator attachment = attachments.find(attachmentHandle); \
        if (attachment == attachments.end()) { \
            VkAttachmentDescription attachmentDesc{}; \
            attachmentDesc.format = WdFormatToVKFormat[it->second.resource.imageResource.image->format]; \
            attachmentDesc.samples = VK_SAMPLE_COUNT_1_BIT; \
            AttachmentInfo info{}; \
            info.attachmentIndex = attachmentIndex; \
            info.attachmentDesc = attachmentDesc; \
            info.presentSrc = it->second.resource.imageResource.image->usage & WdImageUsage::WD_PRESENT; \
            attachmentIndex++; \
            VkAttachmentReference attachmentRef{}; \
            attachmentRef.attachment = info.attachmentIndex; \
            attachmentRef.layout = ATTACH_LAYOUT; \
            info.refs.push_back(attachmentRef); \
            attachments[attachmentHandle] = info; \
            framebuffer.push_back(static_cast<VkImageView>(it->second.resource.imageResource.image->target)); \
            CONTAINER.push_back(attachmentRef); \
        } else { \
            VkAttachmentReference attachmentRef{}; \
            attachmentRef.attachment = attachment->second.attachmentIndex; \
            attachmentRef.layout = ATTACH_LAYOUT; \ 
            attachment->second.refs.push_back(attachmentRef); \
            CONTAINER.push_back(attachmentRef); \
        } \
    }; 

    #define UPDATE_RESOURCES(PARAMS, STAGE) \
    for (std::map<std::string, WdPipeline::ShaderParameter>::iterator it = PARAMS.begin(); it != PARAMS.end(); ++it) { \
                WdPipeline::ShaderParameterType paramType = it->second.paramType; \
                if (paramType & imageMask) { \
                    WdImageHandle handle = it->second.resource.imageResource.image->handle;
                    std::map<WdImageHandle, std::vector<ResourceInfo>>::iterator imgResInfo = imageResources.find(handle); \
                    if (imgResInfo == imageResources.end()) { \
                        std::vector<ResourceInfo> resInfos; \
                        imageResources[handle] = resInfos; \
                    } \
                    ResourceInfo resInfo{}; \
                    resInfo.type = paramType; \
                    resInfo.stage = Stage::STAGE; \
                    resInfo.pipelineIndex = pipelineIndex; \
                    imageResources[handle].push_back(resInfo); \
                } \
                if (paramType & bufferMask) { \
                    WdBufferHandle handle = it->second.resource.bufferResource.buffer->handle; \
                    std::map<WdBufferHandle, std::vector<ResourceInfo>>::iterator bufResInfo = bufferResources.find(handle); \
                    if (bufResInfo == bufferResources.end()) { \
                        std::vector<ResourceInfo> resInfos; \
                        bufferResources[handle] = resInfos; \
                    }
                    ResourceInfo resInfo{}; \
                    resInfo.type = paramType; \
                    resInfo.stage = Stage::STAGE; \
                    resInfo.pipelineIndex = pipelineIndex; \
                    bufferResources[handle].push_back(resInfo); \
                } \
            };

    void VulkanRenderPass::init() {
        using AttachmentInfo = struct AttachmentInfo {
            VkAttachmentDescription attachmentDesc;            
            std::vector<VkAttachmentReference> refs;
            uint8_t attachmentIndex;
            bool presentSrc;
        };

        std::map<WdImageHandle, AttachmentInfo> attachments;

        uint8_t attachmentIndex = 0;

        std::vector<VkImageView> framebuffer;

        std::vector<VkSubpassDescription> subpasses;

        // create subpass descriptions, attachment refs and framebuffer view array 
        // (pipeline <-> subpass being used interchangeably here)
        for (const WdPipeline& pipeline : _pipelines) {
            // There is an imposed ordering for attachments here.
            // Vertex input -> Fragment input -> Fragment Color

            std::vector<VkAttachmentReference> colorRefs;
            std::vector<VkAttachmentReference> inputRefs;
            // resolve, depth, & preserve attachments, idk how to deal with yet 

            // i think input refs can be reused in both shader stages, need to check
            std::map<std::string, WdPipeline::ShaderParameter> vertexInputs = pipeline._vertexParams.subpassInputs;
            UPDATE_ATTACHMENTS(vertexInputs, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, inputRefs)

            std::map<std::string, WdPipeline::ShaderParameter> fragmentInputs = pipeline._fragmentParams.subpassInputs;
            UPDATE_ATTACHMENTS(fragmentInputs, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, inputRefs)

            std::map<std::string, WdPipeline::ShaderParameter> fragmentOutputs = pipeline._fragmentParams.outputs;
            UPDATE_ATTACHMENTS(fragmentOutputs, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL)
            
            VkSubpassDescription subpass{};
            subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
            
            subpass.colorAttachmentCount = static_cast<uint32_t>(colorRefs.size());
            subpass.pColorAttachments = colorRefs.data();

            subpass.inputAttachmentCount = static_cast<uint32_t>(inputRefs.size());
            subpass.pInputAttachments = inputRefs.data();

            //subpass.pDepthStencilAttachment = &depthAttachmentRef;

            subpasses.push_back(subpass);
        };

        // at this point, I have a map of Handle -> AttachmentInfo. Each info has all the uses of an attachment via refs,
        // and everything should be correctly labeled and indexed. Can create the actual descs now & deps now 

        for (std::map<WdHandle, AttachmentInfo>::iterator it = attachments.begin(); it != attachments.end(); ++it) {
            // need to check first and last ref. 
            // if first ref is subpass input, we want to *keep* the values at the end of the renderpass, so the 
            // initial layout should be the same as the final layout and the load op should be load with store op store,
            // otherwise the layout should be undefined and load is clear

            // if the img is being used as present src, then the final layout should be present source,
            // otherwise we match it to the last ref 

            // if present, store op is store, otherwise if the first ref is subpass input we also do store store 

            AttachmentInfo info = it->second;

            VkAttachmentReference& firstRef = info.refs.front().ref;
            VkAttachmentReference& lastRef = info.refs.back().ref;

            VkAttachmentLoadOp loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
            VkAttachmentStoreOp storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;

            VkImageLayout initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
            VkImageLayout finalLayout = info.presentSrc ? VK_IMAGE_LAYOUT_PRESENT_SRC_KHR : lastRef.layout;

            if (firstRef.layout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
                initialLayout = finalLayout;
                loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
                storeOp = VK_ATTACHMENT_STORE_OP_STORE;
            }

            it->second.attachmentDesc.loadOp = loadOp;
            it->second.attachmentDesc.storeOp = storeOp;
            it->second.attachmentDesc.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
            it->second.attachmentDesc.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
            it->second.attachmentDesc.initialLayout = initialLayout;
            it->second.attachmentDesc.finalLayout = finalLayout;

            /*for (const AttachmentRef& ref : info.refs) {

            }*/
        } 

        enum Stage {
            Vertex,
            Fragment,
        };

        // Next need to generate all dependencies now
        using ResourceInfo = struct ResourceInfo {
            WdPipeline::ShaderParamType type;
            Stage stage;
            uint8_t pipelineIndex;
        };

        std::map<WdImageHandle, std::vector<ResourceInfo>> imageResources;
        std::map<WdBufferHandle, std::vector<ResourceInfo>> bufferResources;
        uint8_t pipelineIndex = 0;

        uint32_t bufferMask = WdPipeline::ShaderParameterType::WD_SAMPLED_BUFFER & WdPipeline::ShaderParameterType::WD_BUFFER_IMAGE & WdPipeline::ShaderParameterType::WD_UNIFORM_BUFFER & WdPipeline::ShaderParameterType::WD_STORAGE_BUFFER;
        uint32_t imageMask = WdPipeline::ShaderParameterType::WD_SAMPLED_IMAGE & WdPipeline::ShaderParameterType::WD_TEXTURE_IMAGE & WdPipeline::ShaderParameterType::WD_STORAGE_IMAGE & WdPipeline::ShaderParameterType::WD_SUBPASS_INPUT & WdPipeline::ShaderParameterType::WD_STAGE_OUTPUT;

        // generate resource maps now 
        for (const WdPipeline& pipeline : _pipelines) {
            
            WdPipeline::ShaderParams vertexParams = pipeline._vertexParams;
            UPDATE_RESOURCES(vertexParams.uniforms, Vertex);
            UPDATE_RESOURCES(vertexParams.subpassInputs, Vertex);

            WdPipeline::ShaderParams fragmentParams = pipeline._fragmentParams;
            UPDATE_RESOURCES(fragmentParams.uniforms, Fragment);
            UPDATE_RESOURCES(fragmentParams.subpassInputs, Fragment);
            UPDATE_RESOURCES(fragmentParams.outputs, Fragment);

            pipelineIndex++;
        
        }

        std::vector<VkSubpassDependency> dependencies;
    };

}