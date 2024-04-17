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

    WdImage& VulkanLayer::create2DImage(WdExtent2D extent, uint32_t mipLevels, WdSampleCount sampleCount, WdFormat imageFormat, WdImageUsageFlags usageFlags) {
        VkImage image; // image handle, 1-to-1 translation to GAL 
        VkDeviceMemory imageMemory; // memory handle, 1-to-1
        VkImageView imageView; // view handle, same concept 

        VkImageCreateInfo imageInfo{};
        imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        imageInfo.imageType = VK_IMAGE_TYPE_2D;
        imageInfo.extent = {extent.width, extent.height, 1};
        imageInfo.mipLevels = mipLevels;
        imageInfo.arrayLayers = 1;
        imageInfo.format = WdFormatToVkFormat[format];
        imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL; // start with optimal tiling
        imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED; // should this be undefined to start with?
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

        if (vkCreateImage(_device, &imageInfo, nullptr, &image) != VK_SUCCESS) {
            throw std::runtime_error("Failed to create image!");
        }

        VkMemoryRequirements memRequirements;
        vkGetImageMemoryRequirements(_device, image, &memRequirements);
        
        VkMemoryAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        allocInfo.allocationSize = memRequirements.size;
        allocInfo.memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits, properties);

        if (vkAllocateMemory(_device, &allocInfo, nullptr, &imageMemory) != VK_SUCCESS) {
            throw std::runtime_error("Failed to allocate image memory!");
        }

        vkBindImageMemory(_device, image, imageMemory, 0);

        VkImageViewCreateInfo viewInfo{};
        viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        viewInfo.image = image;
        viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        viewInfo.format = WdFormatToVkFormat[format];

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
       
        if (vkCreateImageView(_device, &viewInfo, nullptr, &imageView) != VK_SUCCESS) {
            throw std::runtime_error("Failed to create 2D image view!");
        }

        // Create GAL resource now 

        WdClearValue clearValue;

        if (flags & VK_IMAGE_ASPECT_DEPTH_BIT) {
            clearValue.depthStencil = defaultDepthStencilClear;
        } else {
            clearValue.color = defaultColorClear;
        }

        WdImage* img = new WdImage(static_cast<WdImageHandle>(image), 
                                   static_cast<WdMemoryPointer>(imageMemory), 
                                   static_cast<WdRenderTarget>(imageView), 
                                   format,
                                   {extent.height, extent.width, 1},
                                   usageFlags,
                                   clearValue);
        // error check etc (will do with cutom allocator in own new operator)

        liveImages.push_back(img); // keep track of this image 

        return *img;
    };

    WdBuffer& VulkanLayer::createBuffer(WdSize size, WdBufferUsageFlags usageFlags) {
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

        if (vkCreateBuffer(_device, &bufferInfo, nullptr, &buffer) != VK_SUCCESS) {
            throw std::runtime_error("Failed to create buffer!");
        }

        VkMemoryRequirements memRequirements;
        vkGetBufferMemoryRequirements(_device, buffer, &memRequirements);

        VkMemoryAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        allocInfo.allocationSize = memRequirements.size;
        allocInfo.memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits, properties);

        if (vkAllocateMemory(_device, &allocInfo, nullptr, &bufferMemory) != VK_SUCCESS) {
            throw std::runtime_error("Failed to allocate buffer memory!");
        }

        vkBindBufferMemory(_device, buffer, bufferMemory, 0);

        // Create GAL resource now 
        WdBuffer* buf = new WdBuffer(static_cast<WdBufferHandle>(buffer), 
                                     static_cast<WdMemoryPointer>(bufferMemory),
                                     size);
        // error check etc (will do with cutom allocator in own new operator)

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

        if (vkCreateSampler(_device, &samplerInfo, nullptr, &textureSampler) != VK_SUCCESS) {
            throw std::runtime_error("Failed to create texture sampler!");
        }

        liveSamplers.push_back(textureSampler);

        return static_cast<WdSamplerHandle>(textureSampler);
    };

    void VulkanLayer::updateBuffer(const WdBuffer& buffer, void *data, WdSize offset, WdSize dataSize) {
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

    void VulkanLayer::waitForFences(const std::vector<WdFenceHandle>& fences, bool waitAll, uint64_t timeout) {
        // should probably do a static cast here for fence data?
        vkWaitForFences(_device, static_cast<uint32_t>(fences.size()), fences.data(), TO_VK_BOOL(waitAll), timeout);
    };

    void VulkanLayer::resetFences(const std::vector<WdFenceHandle>& fences) {
        vkResetFences(_device, static_cast<uint32_t>(fences.size()), fences.data());
    };

    // Skeleton for now, needs to be expanded 
    WdPipeline VulkanLayer::createPipeline(Shader::Shader vertexShader, Shader::Shader fragmentShader, WdViewportProperties viewportProperties) {
        return WdPipeline(vertexShader, fragmentShader, vertexBuilder, viewportProperties);
    };

    // creates render pass and all subpasses 
    WdRenderPass VulkanLayer::createRenderPass(std::vector<WdPipeline> pipelines, std::vector<WdImage> attachments) {
        
    };

    // Render pass creation & utils 

    // usually can just infer from the stage enum in ResourceInfo,
    // but attachments need special attention 
    VkPipelineStageFlags VulkanRenderPass::ResInfoToVkStage(const ResourceInfo& resInfo) const {
        VkPipelineStageFlags stageFlags = 0;
        if (resInfo.type == WdPipeline::ShaderParamType::WD_STAGE_OUTPUT) { // color attachment cant really be both used in fragment and output to in fragment
            stageFlags |= VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        } else if (resInfo.stage & WdStage::Vertex) {
            stageFlags |= VK_PIPELINE_STAGE_VERTEX_SHADER_BIT;
        }
        if (resInfo.stage == WdStage::Fragment) { // but it could be read in vertex
            stageFlags |= VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
        }
        return stageFlags;
    };

    // need to refactor the duplication here 
    void VulkanRenderPass::addDependencies(std::vector<VkSubpassDependency>& dependencies, const std::vector<ResourceInfo>& resInfos, const uint32_t readMask, const uint32_t writeMask) const {
        ResourceInfo lastRead{};
        ResourceInfo lastWrite{};
        lastRead.stage = WdStage::Unknown;
        lastWrite.stage = WdStage::Unknown;
        // Go over every usage of every resource 
        // ugly, need to fix later 
        for (const ResourceInfo& resInfo : resInfos) {
            if (resInfo.type & readMask) {
                if (lastWrite != resInfo && lastWrite.stage != WdStage::Unknown) {
                    // this means there is a write happening before this read, need to 
                    // add a dependency 
                    VkSubpassDependency dependency{};
                    dependency.srcSubpass = lastWrite.pipelineIndex;
                    dependency.dstSubpass = resInfo.pipelineIndex;
                            
                    // stage is obtained from resInfo's stage and param type 
                    dependency.srcStageMask = ResInfoToVkStage(lastWrite);
                    dependency.srcAccessMask = paramTypeToAccess[lastWrite.type];

                    dependency.dstStageMask = ResInfoToVkStage(resInfo);
                    dependency.dstAccessMask = paramTypeToAccess[resInfo.type];
                    dependencies.push_back(dependency);
                }
                lastRead = resInfo;
            }
            if (resInfo.type & writeMask) {
                if (lastRead != resInfo && lastRead.stage != WdStage::Unknown) {
                    // this means there is a write happening before this read, need to 
                    // add a dependency 
                    VkSubpassDependency dependency{};
                    dependency.srcSubpass = lastRead.pipelineIndex;
                    dependency.dstSubpass = resInfo.pipelineIndex;
                            
                    // stage is obtained from resInfo's stage and param type 
                    dependency.srcStageMask = ResInfoToVkStage(lastRead);
                    dependency.srcAccessMask = paramTypeToAccess[lastRead.type];

                    dependency.dstStageMask = ResInfoToVkStage(resInfo);
                    dependency.dstAccessMask = paramTypeToAccess[resInfo.type];
                    dependencies.push_back(dependency);
                }
                lastWrite = resInfo;
            }
        }
    };

    template <class T>
    void VulkanRenderPass::updateAttachements(const T& resourceMap, AttachmentMap& attachments, uint8_t& attachmentIndex, std::vector<VkAttachmentReference>& subpassRefs, std::vector<VkImageView>& framebuffer, VkImageLayout layout) {
        for (T::iterator it = resourceMap.begin(); it != resourceMap.end(); ++it) {
            WdImageHandle attachmentHandle = it->second.resource.image->handle;
            uint8_t index = it->second.decorationIndex;

            AttachmentMap::iterator attachment = attachments.find(attachmentHandle);
            
            if (attachment == attachments.end()) {
                // first time seeing this resource used as an attachment, need to create new entry
                VkAttachmentDescription attachmentDesc{};
                attachmentDesc.format = WdFormatToVkFormat[it->second.resource.image->format];
                attachmentDesc.samples = VK_SAMPLE_COUNT_1_BIT; // need to figure out samples here 
                // layouts, store ops are calculated later

                AttachmentInfo info{}; 
                info.attachmentIndex = attachmentIndex;
                info.attachmentDesc = attachmentDesc;
                info.presentSrc = it->second.resource.image->usage & WdImageUsage::WD_PRESENT;
                    
                attachmentIndex++; 
            
                // add info to attachment map and framebuffers
                attachments[attachmentHandle] = info; 
                framebuffer.push_back(static_cast<VkImageView>(it->second.resource.image->target));
            }

            // create attachment ref 
            VkAttachmentReference attachmentRef{};
            attachmentRef.layout = layout; 
            attachmentRef.attachment = attachments[attachmentHandle].attachmentIndex;

            attachments[attachmentHandle].refs.push_back(attachmentRef);

            subpassRefs[index] = attachmentRef;
        }
    };

    // theres a bit of duplication here that needs refactoring
    void VulkanRenderPass::updateUniformResources(const WdPipeline::Uniforms& resourceMap, ImageResources& imageResources, BufferResources& bufferResources, uint8_t pipelineIndex) {
        for (WdPipeline::Uniforms::iterator it = resourceMap.begin(); it != resourceMap.end(); ++it) {
            WdPipeline::ShaderParameterType paramType = it->second.paramType;
                                                    
            ResourceInfo resInfo{}; 
            resInfo.type = paramType; 
            resInfo.pipelineIndex = pipelineIndex; 
            resInfo.stages = it->second.stages; 

            if (paramType & imageMask) {
                // need to do this for every resource 
                for (const WdPipeline::ShaderResource& res : it->second.resources) {
                    WdImageHandle handle = res.imageResource.image->handle;
                    ImageResources::iterator imgResInfo = imageResources.find(handle); 

                    if (imgResInfo == imageResources.end()) { 
                        std::vector<ResourceInfo> resInfos; 
                        imageResources[handle] = resInfos; 
                    }
                    imageResources[handle].push_back(resInfo); 
                }
            }

            if (paramType & bufferMask) {
                // need to do for every resource
                for (const WdPipeline::ShaderResource& res : it->second.resources) {
                    WdBufferHandle handle = res.bufferResource.buffer->handle; 
                    BufferResources::iterator bufResInfo = bufferResources.find(handle); 
                    
                    if (bufResInfo == bufferResources.end()) { 
                        std::vector<ResourceInfo> resInfos; 
                        bufferResources[handle] = resInfos;
                    }

                    bufferResources[handle].push_back(resInfo); 
                } 
            }
        }
    };

    template <class T>
    void VulkanRenderPass::updateAttachmentResources(const T& resourceMap, ImageResources& imageResources, uint8_t pipelineIndex) {
        for (T::iterator it = resourceMap.begin(); it != resourceMap.end(); ++it) {
            WdPipeline::ShaderParameterType paramType = it->second.paramType;

            ResourceInfo resInfo{}; 
            resInfo.type = paramType; 
            resInfo.stages = WdStage::Fragment; 
            resInfo.pipelineIndex = pipelineIndex; 
            
            WdImageHandle handle = it->second.resource.image->handle;
            ImageResources::iterator imgResInfo = imageResources.find(handle); 
            
            if (imgResInfo == imageResources.end()) { 
                // not found, init array
                std::vector<ResourceInfo> resInfos; 
                imageResources[handle] = resInfos; 
            } 

            imageResources[handle].push_back(resInfo); 
        }
    };

    void VulkanRenderPass::init() {
        AttachmentMap attachments;

        uint8_t attachmentIndex = 0;

        std::vector<VkSubpassDescription> subpasses;

        // create subpass descriptions, attachment refs and framebuffer view array 
        // (pipeline <-> subpass being used interchangeably here)
        for (const WdPipeline& pipeline : _pipelines) {
            // There is an imposed ordering for attachments here.
            // Fragment input -> Fragment Color

            // resolve, depth, & preserve attachments, idk how to deal with yet 

            WdPipeline::SubpassInputs fragmentInputs = pipeline._subpassInputs;
            WdPipeline::FragmentOutputs fragmentOutputs = pipeline._fragmentOutputs;

            std::vector<VkAttachmentReference> inputRefs(fragmentInputs.size());
            std::vector<VkAttachmentReference> colorRefs(fragmentOutputs.size());

            updateAttachements<WdPipeline::SubpassInputs>(fragmentInputs, attachments, attachmentIndex, inputRefs, _framebuffer, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
            updateAttachements<WdPipeline::FragmentOutputs>(fragmentOutputs, attachments, attachmentIndex, colorRefs, _framebuffer, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
            
            VkSubpassDescription subpass{};
            subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
            //TODO: deal with unused attachments here
            subpass.colorAttachmentCount = static_cast<uint32_t>(colorRefs.size());
            subpass.pColorAttachments = colorRefs.data();

            subpass.inputAttachmentCount = static_cast<uint32_t>(inputRefs.size());
            subpass.pInputAttachments = inputRefs.data();

            // TODO: depth here 
            //subpass.pDepthStencilAttachment = &depthAttachmentRef;

            subpasses.push_back(subpass);
        };

        // at this point, I have a map of Handle -> AttachmentInfo. Each info has all the uses of an attachment via refs,
        // and everything should be correctly labeled and indexed. Can create the actual descs now & deps now 

        std::vector<VkAttachmentDescription> attachmentDescs(attachments.size());

        for (AttachmentMap::iterator it = attachments.begin(); it != attachments.end(); ++it) {
            // need to check first and last ref. 
            // if first ref is subpass input, we want to *keep* the values at the end of the renderpass, so the 
            // initial layout should be the same as the final layout and the load op should be load with store op store,
            // otherwise the layout should be undefined and load is clear

            // if the img is being used as present src, then the final layout should be present source,
            // otherwise we match it to the last ref 

            // if present, store op is store, otherwise if the first ref is subpass input we also do store store 

            AttachmentInfo info = it->second;

            VkAttachmentReference& firstRef = info.refs.front();
            VkAttachmentReference& lastRef = info.refs.back();

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

            attachmentDescs[info.attachmentIndex] = it->second.attachmentDesc;
        } 
        
        // need to generate all deps now 

        ImageResources imageResources;
        BufferResources bufferResources;

        uint8_t pipelineIndex = 0;
        // generate resource maps now 
        for (const WdPipeline& pipeline : _pipelines) {
            updateUniformResources(pipeline._uniforms, imageResources, bufferResources, pipelineIndex);

            // Fragment-only subpass inputs and outs
            WdPipeline::SubpassInputs fragmentInputs = pipeline._subpassInputs;
            WdPipeline::FragmentOutputs fragmentOutputs = pipeline._fragmentOutputs;

            updateAttachmentResources<WdPipeline::SubpassInputs>(fragmentInputs, imageResources, pipelineIndex);
            updateAttachmentResources<WdPipeline::FragmentOutputs>(fragmentOutputs, imageResources, pipelineIndex);

            pipelineIndex++;
        }

        std::vector<VkSubpassDependency> dependencies;

        for (ImageResources::iterator it = imageResources.begin(); it != imageResources.end(); ++it) {
            addDependencies(dependencies, it->second, imgReadMask, imgWriteMask);
        }

        for (BufferResources::iterator it = bufferResources.begin(); it != bufferResources.end(); ++it) {
            addDependencies(dependencies, it->second, bufReadMask, bufWriteMask);
        }
        // all dependencies except external dependencies should be added now 

        // can create the actual render pass object now 
        VkRenderPassCreateInfo renderPassInfo{};
        renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
        renderPassInfo.attachmentCount = static_cast<uint32_t>(attachmentDescs.size());
        renderPassInfo.pAttachments = attachmentDescs.data();
        renderPassInfo.subpassCount = static_cast<uint32_t>(subpasses.size());
        renderPassInfo.pSubpasses = subpasses.data();
        renderPassInfo.dependencyCount = static_cast<uint32_t>(dependencies.size());
        renderPassInfo.pDependencies = dependencies.data();

        if (vkCreateRenderPass(_device, &renderPassInfo, nullptr, &_renderPass) != VK_SUCCESS) {
            throw std::runtime_error("Failed to create render pass");
        }

        createDescriptorPool(); // create descriptor pool first, then allocate sets when creating pipeline 

        // create all Vulkan pipelines now 
        uint8_t index = 0;
        for (const WdPipeline& pipeline : _pipelines) {
            VulkanPipeline vkPipeline = createVulkanPipeline(pipeline, index);
            writeDescriptorSets(vkPipeline.descriptorSets, pipeline._uniforms, pipeline._subpassInputs);
            _vkPipelines.push_back(vkPipeline);
            index++;
        }
    };

    // Vulkan Pipeline, layout and descriptor set creation 

    void VulkanRenderPass::writeDescriptorSets(const std::vector<VkDescriptorSet>& descriptorSets, const WdPipeline::Uniforms& uniforms, const WdPipeline::SubpassInputs& subpassInputs) const {
        for (const VkDescriptorSet& descriptorSet : descriptorSets) {
            std::vector<VkWriteDescriptorSet> descriptorWrites{};

            for (const WdPipeline::Uniform& uniform : uniforms) {
                
                uint8_t arrayIndex = 0;
                for (const WdPipeline::ShaderResource& shaderResource : uniform.resources) {
                    VkWriteDescriptorSet descriptorWrite{};
                    
                    descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
                    descriptorWrite.dstSet = descriptorSet; // TODO: can handle multiple frames in flight this way ?
                    descriptorWrite.dstBinding = uniform.decorationBinding;
                    descriptorWrite.dstArrayElement = arrayIndex; // set elements 1 by 1 
                    descriptorWrite.descriptorType = WdParamTypeToVkDescriptorType[uniform.paramType];
                    descriptorWrite.descriptorCount = 1; // static_cast<uint32_t>(uniform.resources.size());
                    descriptorWrite.pBufferInfo = nullptr;
                    descriptorWrite.pImageInfo = nullptr;
                    descriptorWrite.pTexelBufferView = nullptr;

                    if (uniform.paramType & _imageMask) {
                        VkDescriptorImageInfo imageInfo{};
                        imageInfo.sampler = static_cast<VkSampler>(shaderResource.imageResource.sampler);
                        imageInfo.imageView = static_cast<VkImageView>(shaderResource.imageResource.image->target); // TODO:: write translate function for this 
                        imageInfo.layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL; // TODO:: i think this needs fixing as well 

                        descriptorWrite.pImageInfo = &imageInfo;
                    }

                    if (uniform.paramType & _bufferMask) {
                        if (uniform.paramType == WdPipeline::ShaderParameterType::WD_BUFFER_IMAGE || uniform.paramType == WdPipeline::ShaderParameterType::WD_SAMPLED_BUFFER) {
                            descriptorWrite.pTexelBufferView = &(shaderResource.bufferResource.bufferTarget); // this might be a problem, might have to cast first to a variable then pass reference 
                        } else {
                            VkDescriptorBufferInfo bufferInfo{};
                            bufferInfo.buffer = static_cast<VkBuffer>(shaderResource.bufferResource.buffer->handle);
                            bufferInfo.offset = 0; //TODO: idk when it wouldn't be 
                            bufferInfo.range = static_cast<VkDeviceSize>(shaderResource.bufferResource.buffer->size); 

                            descriptorWrite.pBufferInfo = &bufferInfo;
                        }
                    }

                    descriptorWrites.push_back(descriptorWrite);

                    arrayIndex++;
                }
            }

            for (const WdPipeline::SubpassInput& subpassInput : subpassInputs) {
                VkDescriptorImageInfo imageInfo{};
                imageInfo.sampler = nullptr; // no sampler 
                imageInfo.imageView = static_cast<VkImageView>(subpassInput.resource.image->target); // TODO:: write translate function for this 
                imageInfo.layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL; 

                VkWriteDescriptorSet descriptorWrite{};

                descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
                descriptorWrite.dstSet = descriptorSet; // TODO: can handle multiple frames in flight this way ?
                descriptorWrite.dstBinding = uniform.decorationBinding;
                descriptorWrite.dstArrayElement = 0; // subpass inputs can't be in an array 
                descriptorWrite.descriptorType = WdParamTypeToVkDescriptorType[uniform.paramType];
                descriptorWrite.descriptorCount = 1; // static_cast<uint32_t>(uniform.resources.size());
                descriptorWrite.pBufferInfo = nullptr;
                descriptorWrite.pImageInfo = &imageInfo;
                descriptorWrite.pTexelBufferView = nullptr;

                descriptorWrites.push_back(descriptorWrite);
            }

            vkUpdateDescriptorSets(_device, static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);
        }
    };

    
    VkShaderStageFlagBits VulkanRenderPass::WdStagesToVkStages(const WdStageMask stages) const {
        VkShaderStageFlagBits shaderStages = 0;
        if (stages & WdStage::Vertex) {
            shaderStages |= VK_SHADER_STAGE_VERTEX_BIT;
        }
        if (stages & WdStage::Fragment) {
            shaderStages |= VK_SHADER_STAGE_FRAGMENT_BIT;
        }
        return shaderStages;
    };


    VkDescriptorSetLayout VulkanRenderPass::createDescriptorSetLayout(const WdPipeline::Uniforms& uniforms, const WdPipeline::SubpassInputs& subpassInputs) {
        VkDescriptorSetLayout layout;
        
        std::vector<VkDescriptorSetLayoutBinding> bindings;

        for (WdPipeline::Uniforms::iterator it = uniforms.begin(); it != uniforms.end(); ++it) {
            WdPipeline::Uniform uniform = it->second;
            
            VkDescriptorSetLayoutBinding binding;
            binding.binding = uniform.decorationBinding;
            binding.descriptorType = WdParamTypeToVkDescriptorType[uniform.paramType];
            binding.descriptorCount = uniform.resourceCount;
            binding.stageFlags = WdStagesToVkStages(uniform.stages);
            binding.pImmutableSamplers = nullptr; // TODO

            bindings.push_back(binding);
        }
        
        // This is a bit of duplication...
        for (WdPipeline::SubpassInputs::iterator it = subpassInputs.begin(); it != subpassInputs.end(); ++it) {
            WdPipeline::SubpassInput subpassInput = it->second;
            
            VkDescriptorSetLayoutBinding binding;
            binding.binding = uniform.decorationBinding;
            binding.descriptorType = WdParamTypeToVkDescriptorType[uniform.paramType];
            binding.descriptorCount = 1; // Fixed size 1 
            binding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT; // subpass inputs are fragment only 
            binding.pImmutableSamplers = nullptr; // TODO

            bindings.push_back(binding);
        }

        VkDescriptorSetLayoutCreateInfo layoutInfo{};
        layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
        layoutInfo.pBindings = bindings.data();

        if (vkCreateDescriptorSetLayout(_device, &layoutInfo, nullptr, &layout) != VK_SUCCESS) {
            throw std::runtime_error("Failed to create descriptor set layout!");
        }

        return layout;
    };  

    // TODO: using vector a lot everywhere, I should look into performance characteristics,
    // maybe arrays or other stack-based collections are better
    VertexInputDesc VulkanRenderPass::createVertexAttributeDescriptionsAndBinding(const WdPipeline::VertexInputs& vertexInputs) const {
        std::vector<VkVertexInputAttributeDescription> attributeDescriptions;
        
        VkVertexInputBindingDescription bindingDescription{};

        bindingDescription.binding = 0; // TODO: how to handle this? In what case do we have different bindings?
        bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX; // when is this instance?
        bindingDescription.stride = 0;

        for (const WdPipeline::VertexInput& vertexInput : vertexInputs) { 
            
            bindingDescription.stride += vertexInput.size; // TODO: more in depth calculations might be needed here because of alignment and offsets, can query with SPIRV reflect 

            VkVertexInputAttributeDescription attributeDescription;
            attributeDescription.binding = 0; // TODO: same as above
            attributeDescription.location = vertexInput.decorationLocation;
            attributeDescription.format = WdFormatToVkFormat[vertexInput.format];
            attributeDescription.offset = vertexInput.offset;

            attributeDescriptions.push_back(attributeDescription);
        }

        return {attributeDescriptions, bindingDescription};
    };

    VulkanPipeline VulkanRenderPass::createVulkanPipeline(const WdPipeline& pipeline, const uint8_t index) {
        VulkanPipeline vkPipeline{};
        vkPipeline.descriptorSetLayouts.push_back(createDescriptorSetLayout(pipeline._uniforms, pipeline._subpassInputs)); // support only one layout for now 
        
        VkDescriptorSetAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        allocInfo.descriptorPool = _descriptorPool;
        allocInfo.descriptorSetCount = static_cast<uint32_t>(vkPipeline.descriptorSetLayouts.size()); // TODO: Max frames in flight thing here 
        allocInfo.pSetLayouts = vkPipeline.descriptorSetLayouts.data();

        vkPipeline.descriptorSets.resize(vkPipeline.descriptorSetLayouts.size());
        if (vkAllocateDescriptorSets(_device, &allocInfo, vkPipeline.descriptorSets.data()) != VK_SUCCESS) {
            throw std::runtime_error("Failed to allocate descriptor sets when creating VK pipelines!");
        }

        VkShaderModule vertShaderModule = createShaderModule(pipeline._vertexShader);
        VkShaderModule fragShaderModule = createShaderModule(pipeline._fragmentShader);

        VkPipelineShaderStageCreateInfo vertShaderStageInfo{};
        vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
        vertShaderStageInfo.module = vertShaderModule;
        vertShaderStageInfo.pName = "main"; // TODO: could be configurable like Unreal
        vertShaderStageInfo.pNext = nullptr;

        VkPipelineShaderStageCreateInfo fragShaderStageInfo{};
        fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
        fragShaderStageInfo.module = fragShaderModule;
        fragShaderStageInfo.pName = "main"; // TODO: could be configurable like Unreal
        fragShaderStageInfo.pNext = nullptr;

        VkPipelineShaderStageCreateInfo shaderStages[] = {
            vertShaderStageInfo, fragShaderStageInfo
        };

        VertexInputDesc vertexInputDesc = createVertexAttributeDescriptionsAndBinding(pipeline._vertexInputs;);

        VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
        vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
        vertexInputInfo.vertexBindingDescriptionCount = 1; // TODO: same as in binding desc function
        vertexInputInfo.pVertexBindingDescriptions = &std::get<1>(vertexInputDesc);
        vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(std::get<0>(vertexInputDesc).size());
        vertexInputInfo.pVertexAttributeDescriptions = std::get<0>(vertexInputDesc).data();

        VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
        inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
        inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
        inputAssembly.primitiveRestartEnable = VK_FALSE;

        // Dynamic state
        VkViewport viewport{};
        viewport.x = static_cast<float>(pipeline._viewportProperties.startCoords.x);
        viewport.y = static_cast<float>(pipeline._viewportProperties.startCoords.y);
        viewport.width = static_cast<float>(pipeline._viewportProperties.endCoords.x);
        viewport.height = static_cast<float>(pipeline._viewportProperties.endCoords.y);
        viewport.minDepth = pipeline._viewportProperties.depth.min;
        viewport.maxDepth = pipeline._viewportProperties.depth.max;

        VkRect2D scissor{};
        scissor.offset = pipeline._viewportProperties.scissor.offset;
        scissor.extent = pipeline._viewportProperties.scissor.extent;

        std::vector<VkDynamicState> dynamicStates = {
            VK_DYNAMIC_STATE_VIEWPORT, 
            VK_DYNAMIC_STATE_SCISSOR
        };

        VkPipelineDynamicStateCreateInfo dynamicState{};
        dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
        dynamicState.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size());
        dynamicState.pDynamicStates = dynamicStates.data();
        
        VkPipelineViewportStateCreateInfo viewportState{};
        viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
        viewportState.viewportCount = 1;
        viewportState.scissorCount = 1;

        //TODO: Currently rasterizer, multisampling & color blending are not customizable
        VkPipelineRasterizationStateCreateInfo rasterizer{};
        rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
        rasterizer.depthClampEnable = VK_FALSE;
        rasterizer.rasterizerDiscardEnable = VK_FALSE;
        rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
        rasterizer.lineWidth = 1.0f;
        rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
        rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;

        rasterizer.depthBiasEnable = VK_FALSE;
        rasterizer.depthBiasConstantFactor = 0.0f;
        rasterizer.depthBiasClamp = 0.0f;
        rasterizer.depthBiasSlopeFactor = 0.0f;

        VkPipelineMultisampleStateCreateInfo multisampling{};
        multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
        multisampling.sampleShadingEnable = VK_FALSE;
        multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
        multisampling.minSampleShading = 1.0f;
        multisampling.pSampleMask = nullptr;
        multisampling.alphaToCoverageEnable = VK_FALSE;
        multisampling.alphaToOneEnable = VK_FALSE;

        // Do alpha blending
        // This is the blend attachment state, per framebuffer. 

        VkPipelineColorBlendAttachmentState colorBlendAttachment{};
        colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | 
                                            VK_COLOR_COMPONENT_G_BIT | 
                                            VK_COLOR_COMPONENT_B_BIT |
                                            VK_COLOR_COMPONENT_A_BIT;
        colorBlendAttachment.blendEnable = VK_FALSE;
        colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
        colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
        colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
        colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
        colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
        colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;

        // Add color blend here for every color output
        // TODO: color blending for depth here?
        std::vector<VkPipelineColorBlendAttachmentState> blendStates(pipeline._fragmentOutputs.size(), colorBlendAttachment);

        // Color Blend State, for all framebuffers 

        VkPipelineColorBlendStateCreateInfo colorBlending{};
        colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
        colorBlending.logicOpEnable = VK_FALSE;
        colorBlending.logicOp = VK_LOGIC_OP_COPY;
        colorBlending.attachmentCount = static_cast<uint32_t>(blendStates.size());
        colorBlending.pAttachments = blendStates.data();
        colorBlending.blendConstants[0] = 0.0f;
        colorBlending.blendConstants[1] = 0.0f;
        colorBlending.blendConstants[2] = 0.0f;
        colorBlending.blendConstants[3] = 0.0f;

        VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
        pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        pipelineLayoutInfo.setLayoutCount = static_cast<uint32_t>(vkPipeline.descriptorSetLayouts.size()); // TODO: when would you have more than 1?
        pipelineLayoutInfo.pSetLayouts = vkPipeline.descriptorSetLayouts.data();
        pipelineLayoutInfo.pushConstantRangeCount = 0; // TODO: work push constants into the workflow
        pipelineLayoutInfo.pPushConstantRanges = nullptr;

        if (vkCreatePipelineLayout(_device, &pipelineLayoutInfo, nullptr, &vkPipeline.pipelineLayout) != VK_SUCCESS) {
            throw std::runtime_error("Failed to create G-Buffer pipeline layout!");
        }

        // TODO: do this based on whether the depth resource is set or not 
        VkPipelineDepthStencilStateCreateInfo depthStencil{};
        depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
        depthStencil.depthTestEnable = VK_TRUE;
        depthStencil.depthWriteEnable = VK_TRUE;
        depthStencil.depthCompareOp = VK_COMPARE_OP_LESS;
       
        depthStencil.depthBoundsTestEnable = VK_FALSE;
        depthStencil.minDepthBounds = 0.0f;
        depthStencil.maxDepthBounds = 1.0f;

        // TODO: figure out stencil usage 
        depthStencil.stencilTestEnable = VK_FALSE;
        depthStencil.front = {};
        depthStencil.back = {};

        VkGraphicsPipelineCreateInfo pipelineInfo{};
        pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
        pipelineInfo.stageCount = 2;
        pipelineInfo.pStages = shaderStages;

        pipelineInfo.pVertexInputState = &vertexInputInfo;
        pipelineInfo.pInputAssemblyState = &inputAssembly;
        pipelineInfo.pViewportState = &viewportState;
        pipelineInfo.pRasterizationState = &rasterizer;
        pipelineInfo.pMultisampleState = &multisampling;
        pipelineInfo.pDepthStencilState = &depthStencil;
        pipelineInfo.pColorBlendState = &colorBlending;
        pipelineInfo.pDynamicState = &dynamicState;
        pipelineInfo.layout = vkPipeline.pipelineLayout;
        pipelineInfo.renderPass = _renderPass;
        pipelineInfo.subpass = index;
        pipelineInfo.basePipelineHandle = VK_NULL_HANDLE; // TODO: deal with this during pipeline caching
        //pipelineInfo.basePipelineIndex = -1;

        if (vkCreateGraphicsPipelines(_device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &vkPipeline.pipeline) != VK_SUCCESS) {
            throw std::runtime_error("Failed to create G-Buffer graphics pipeline");
        }
        
        vkDestroyShaderModule(device, fragShaderModule, nullptr);
        vkDestroyShaderModule(device, vertShaderModule, nullptr);    

        return VulkanPipeline;
    };

    // Create descriptor pool 

    void VulkanRenderPass::addDescriptorPoolSizes(const WdPipeline& pipeline, std::vector<VkDescriptorPoolSize>& poolSizes) const {
        // according to Vulkan spec this is fine to do 
        for (WdPipeline::Uniforms::iterator it = pipeline._uniforms.begin(); it != pipeline._uniforms.end(); ++it) {
            VkDescriptorPoolSize poolSize{};
            poolSize.type = WdParamTypeToVkDescriptorType[it->second.paramType];
            poolSize.descriptorCount = static_cast<uint32_t>(it->second.resourceCount);
            poolSizes.push_back(poolSize);
        };

        for (WdPipeline::SubpassInputs::iterator it = pipeline._subpassInputs.begin(); it != pipeline._subpassInputs.end(); ++it) {
            VkDescriptorPoolSize poolSize{};
            poolSize.type = WdParamTypeToVkDescriptorType[it->second.paramType];
            poolSize.descriptorCount = 1; // Fixed to 1 
            poolSizes.push_back(poolSize);
        };
    };
    
    void VulkanRenderPass::createDescriptorPool() {
        std::vector<VkDescriptorPoolSize> poolSizes{};

        for (const WdPipeline& pipeline : _pipelines) {
            addDescriptorPoolSizes(pipeline, poolSizes);
        }

        VkDescriptorPoolCreateInfo poolInfo{};
        poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
        poolInfo.pPoolSizes = poolSizes.data();
        poolInfo.maxSets = static_cast<uint32_t>(pipelines.size()); // as many as pipelines, what does that mean?
        poolInfo.flags = 0;

        if (vkCreateDescriptorPool(_device, &poolInfo, nullptr, &_descriptorPool) != VK_SUCCESS) {
            throw std::runtime_error("Failed to create descriptor pool");
        }
    }

}