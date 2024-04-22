#include "VulkanRenderPass.h"

namespace Wado::GAL::Vulkan {

    // const VkAccessFlags VulkanRenderPass::WdShaderParamTypeToAccess[] = {
    //             VK_ACCESS_SHADER_READ_BIT, 
    //             VK_ACCESS_SHADER_READ_BIT,
    //             VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT,
    //             VK_ACCESS_SHADER_READ_BIT,
    //             VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT,
    //             VK_ACCESS_UNIFORM_READ_BIT,
    //             VK_ACCESS_INPUT_ATTACHMENT_READ_BIT,
    //             VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT,
    //             VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
    // };

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
    };

    // TODO: Util, move to Vulkan Layer
    static VkClearValue WdClearValueToVkClearValue(WdClearValue clearValue, bool isDepthStencil = false) {
        VkClearValue vkClearValue{};
        if (isDepthStencil) {
            vkClearValue.depthStencil.depth = clearValue.depthStencil.depth; // direct translation
            vkClearValue.depthStencil.stencil = clearValue.depthStencil.stencil;
        } else {
            vkClearValue.color = {{clearValue.color.r, clearValue.color.g, clearValue.color.b, clearValue.color.a}};
        }
        return vkClearValue;
    };

    // TODO: a bit of repetition in all the update functions 
    void VulkanRenderPass::updateFragmentOutputAttachements(const VulkanPipeline::VkFragmentOutputs& resourceMap, AttachmentMap& attachments, uint8_t& attachmentIndex, std::vector<VkAttachmentReference>& attachmentRefs, Framebuffer& framebuffer, VkImageLayout layout) {
        for (VulkanPipeline::VkFragmentOutputs::const_iterator it = resourceMap.begin(); it != resourceMap.end(); ++it) {
            
            WdImageHandle attachmentHandle = it->second.resource->handle;
            uint8_t attachmentArrayIndex = it->second.decorationLocation; // Thsi defines where this attachment ref should be placed
            // in pColorAttachments, pInputAttachments, etc.

            AttachmentMap::iterator attachment = attachments.find(attachmentHandle);
            
            if (attachment == attachments.end()) {
                // First time seeing this resource used as an attachment, need to create new entry
                VkAttachmentDescription attachmentDesc{};
                attachmentDesc.format = WdFormatToVkFormat[it->second.resource->format];
                attachmentDesc.samples = VK_SAMPLE_COUNT_1_BIT; // TODO :: Figure out how to deal with samples. 
                // layouts, store and load ops are calculated later

                AttachmentInfo info{}; 
                info.attachmentIndex = attachmentIndex;
                info.attachmentDesc = attachmentDesc;
                info.presentSrc = it->second.resource->usage & WdImageUsage::WD_PRESENT;
                    
                attachmentIndex++; 
            
                // Add info to attachment map and framebuffers
                attachments[attachmentHandle] = info; 
                // Framebuffer location maps to attachment index 
                framebuffer.attachmentViews.push_back(static_cast<VkImageView>(it->second.resource->target));
                framebuffer.clearValues.push_back(WdClearValueToVkClearValue(it->second.resource->clearValue));
            };

            // Create attachment ref 
            VkAttachmentReference attachmentRef{};
            attachmentRef.layout = layout; 
            attachmentRef.attachment = attachments[attachmentHandle].attachmentIndex;

            attachments[attachmentHandle].refs.push_back(attachmentRef);

            attachmentRefs[attachmentArrayIndex] = attachmentRef;
        }; 
    };

    void VulkanRenderPass::updateSubpassInputAttachements(const VulkanPipeline::VkSubpassInputs& resourceMap, AttachmentMap& attachments, uint8_t& attachmentIndex, std::vector<VkAttachmentReference>& attachmentRefs, Framebuffer& framebuffer, VkImageLayout layout) {
        for (VulkanPipeline::VkSubpassInputs::const_iterator it = resourceMap.begin(); it != resourceMap.end(); ++it) {
            
            WdImageHandle attachmentHandle = it->second.resource->handle;
            uint8_t attachmentArrayIndex = it->second.decorationIndex;

            AttachmentMap::iterator attachment = attachments.find(attachmentHandle);
            
            if (attachment == attachments.end()) {
                // First time seeing this resource used as an attachment, need to create new entry
                VkAttachmentDescription attachmentDesc{};
                attachmentDesc.format = WdFormatToVkFormat[it->second.resource->format];
                attachmentDesc.samples = VK_SAMPLE_COUNT_1_BIT; // TODO :: Figure out how to deal with samples. 
                // layouts, store and load ops are calculated later

                AttachmentInfo info{}; 
                info.attachmentIndex = attachmentIndex;
                info.attachmentDesc = attachmentDesc;
                info.presentSrc = it->second.resource->usage & WdImageUsage::WD_PRESENT;
                    
                attachmentIndex++; 
            
                // Add info to attachment map and framebuffers
                attachments[attachmentHandle] = info;
                // Framebuffer location maps to attachment index  
                framebuffer.attachmentViews.push_back(static_cast<VkImageView>(it->second.resource->target));
                framebuffer.clearValues.push_back(WdClearValueToVkClearValue(it->second.resource->clearValue));
            };

            // Create attachment ref 
            VkAttachmentReference attachmentRef{};
            attachmentRef.layout = layout; 
            attachmentRef.attachment = attachments[attachmentHandle].attachmentIndex;

            attachments[attachmentHandle].refs.push_back(attachmentRef);

            attachmentRefs[attachmentArrayIndex] = attachmentRef;
        }; 
    };

    void VulkanRenderPass::updateDepthStencilAttachment(const Memory::WdClonePtr<WdImage> depthStencilAttachment, AttachmentMap& attachments, uint8_t& attachmentIndex, Framebuffer& framebuffer, VkSubpassDescription& subpass) {
        if (!depthStencilAttachment) { // invalid pointer, no depth attachment resource set
            return;
        };

        // If not, look if this image has been used before
        WdImageHandle attachmentHandle = depthStencilAttachment->handle;
        AttachmentMap::iterator attachment = attachments.find(attachmentHandle);

        if (attachment == attachments.end()) { // Not found, needs desc.
             // First time seeing this resource used as an attachment, need to create new entry
            VkAttachmentDescription attachmentDesc{};
            attachmentDesc.format = WdFormatToVkFormat[depthStencilAttachment->format];
            attachmentDesc.samples = VK_SAMPLE_COUNT_1_BIT; // TODO :: Figure out how to deal with samples. 
            // layouts, store and load ops are calculated later

            AttachmentInfo info{}; 
            info.attachmentIndex = attachmentIndex;
            info.attachmentDesc = attachmentDesc;
            info.presentSrc = depthStencilAttachment->usage & WdImageUsage::WD_PRESENT;
                    
            attachmentIndex++; 
            
            // Add info to attachment map and framebuffers
            attachments[attachmentHandle] = info;
            // Framebuffer location maps to attachment index  
            framebuffer.attachmentViews.push_back(static_cast<VkImageView>(depthStencilAttachment->target));
            framebuffer.clearValues.push_back(WdClearValueToVkClearValue(depthStencilAttachment->clearValue, true));
        };

        // Create attachment ref 
        VkAttachmentReference attachmentRef{};
        attachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL; 
        attachmentRef.attachment = attachments[attachmentHandle].attachmentIndex;

        attachments[attachmentHandle].refs.push_back(attachmentRef);

        // Update subpass description now.
        subpass.pDepthStencilAttachment = &attachmentRef; // TODO: I'm pretty sure the ref dies? 
    };

    // UTILS, move to VkLayer
    static inline bool isImageDescriptor(VkDescriptorType descType) {
        return descType >= VK_DESCRIPTOR_TYPE_SAMPLER && descType <= VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
    };

    static inline bool isBufferDescriptor(VkDescriptorType descType) {
        return descType >= VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER && descType <= VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC;
    };
    // END OF UTILS 

    void VulkanRenderPass::updateUniformResources(const VulkanPipeline::VkUniforms& resourceMap, ImageResources& imageResources, BufferResources& bufferResources, const uint8_t pipelineIndex, DescriptorCounts& descriptorCounts) {
        for (VulkanPipeline::VkUniforms::const_iterator it = resourceMap.begin(); it != resourceMap.end(); ++it) {
            VkDescriptorType descType = it->second.descType;
            // We are checking every uniform in a pipeline here, so it's worth also updating descriptor counts
            // for pool creation later on.

            if (descriptorCounts.find(descType) == descriptorCounts.end()) {
                descriptorCounts[descType] = 0;
            };
            descriptorCounts[descType] += it->second.resourceCount;
            // This is analogous to the attachment ref earlier      
            ResourceInfo resInfo{}; 
            resInfo.type = descType; 
            resInfo.pipelineIndex = pipelineIndex; 
            resInfo.stages = it->second.stages; 

            if (isImageDescriptor(descType)) {
                // Need to do this for every resource 
                for (const WdShaderResource& res : it->second.resources) {
                    WdImageHandle handle = res.imageResource.image->handle;
                    ImageResources::iterator imgResInfo = imageResources.find(handle); 

                    if (imgResInfo == imageResources.end()) { 
                        // This resource has never been seen before
                        std::vector<ResourceInfo> resInfos; 
                        imageResources[handle] = resInfos; 
                    };
            
                    imageResources[handle].push_back(resInfo); 
                };
            };
            // TODO: the two if's here are quite ugly, find a better way
            // Idea: might not need to differentiate resource types..., just need handle to stage usage and Read/Write. 
            if (isBufferDescriptor(descType)) {
                // need to do for every resource
                for (const WdShaderResource& res : it->second.resources) {
                    WdBufferHandle handle = res.bufferResource.buffer->handle; 
                    BufferResources::iterator bufResInfo = bufferResources.find(handle); 
                    
                    if (bufResInfo == bufferResources.end()) {
                        // This resource has never been seen before 
                        std::vector<ResourceInfo> resInfos; 
                        bufferResources[handle] = resInfos;
                    };

                    bufferResources[handle].push_back(resInfo); 
                };
            };
        }        
    };

    template <class T>
    void VulkanRenderPass::updateAttachmentResources(const T& resourceMap, ImageResources& imageResources, const VkDescriptorType type, const uint8_t pipelineIndex) {
        for (T::iterator it = resourceMap.begin(); it != resourceMap.end(); ++it) {

            ResourceInfo resInfo{}; 
            resInfo.type = type;
            resInfo.stages = VK_SHADER_STAGE_FRAGMENT_BIT; 
            resInfo.pipelineIndex = pipelineIndex; 
            
            WdImageHandle handle = it->second.resource->handle;
            ImageResources::iterator imgResInfo = imageResources.find(handle); 
            
            if (imgResInfo == imageResources.end()) { 
                // not found, init array
                std::vector<ResourceInfo> resInfos; 
                imageResources[handle] = resInfos; 
            };

            imageResources[handle].push_back(resInfo); 
        };
    };

    // TODO: should util functions all be moved to VkLayer?
    bool VulkanRenderPass::isWriteDescriptorType(VkDescriptorType type) {
        return type == VK_DESCRIPTOR_TYPE_STORAGE_IMAGE ||
            type == VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER || 
            type == VK_DESCRIPTOR_TYPE_STORAGE_BUFFER ||
            type == VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC ||
            type == FRAGMENT_OUTPUT_DESC;
    };

    VkPipelineStageFlags VulkanRenderPass::resInfoToVkStage(const ResourceInfo& resInfo) {
        VkPipelineStageFlags stageFlags = 0;

        if (resInfo.type == FRAGMENT_OUTPUT_DESC) { // color attachment cant really be both used in fragment and output to in fragment
            stageFlags |= VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        } else if (resInfo.stages & VK_SHADER_STAGE_FRAGMENT_BIT) {
            stageFlags |= VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
        }

        if (resInfo.stages == VK_SHADER_STAGE_VERTEX_BIT) { // but it could be read in vertex
            stageFlags |= VK_PIPELINE_STAGE_VERTEX_SHADER_BIT;
        }

        return stageFlags;
    };

    // TODO: this doesn't want to be const becuase of return type
    std::map<VkDescriptorType, VkAccessFlags> VulkanRenderPass::decriptorTypeToAccessType = {
            {VK_DESCRIPTOR_TYPE_SAMPLER, VK_ACCESS_SHADER_READ_BIT}, 
            {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_ACCESS_SHADER_READ_BIT},
            {VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, VK_ACCESS_SHADER_READ_BIT},
            {VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT},
            {VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, VK_ACCESS_SHADER_READ_BIT},
            {VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT},
            {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_ACCESS_UNIFORM_READ_BIT},
            {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT},
            {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, VK_ACCESS_UNIFORM_READ_BIT},
            {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT},
            {VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, VK_ACCESS_INPUT_ATTACHMENT_READ_BIT},
            {FRAGMENT_OUTPUT_DESC, VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT},
        };

    // need to refactor the duplication here 
    void VulkanRenderPass::addDependencies(std::vector<VkSubpassDependency>& dependencies, const std::vector<ResourceInfo>& resInfos) {
        std::vector<ResourceInfo> previousReads{};
        ResourceInfo lastWrite{};
        lastWrite.stages = VkShaderStageFlagBits::VK_SHADER_STAGE_ALL; // TODO: Should I change it so every time I use enums I mention the scope?
        lastWrite.pipelineIndex = -1;
        // Go over every usage of every resource 
        // All reads after a write must wait for the write to finish.
        // A new write must wait for *all* previous reads inbetween it and the last write to finish
        // Reset read vector when finding a new write.
        for (const ResourceInfo& resInfo : resInfos) {
            // All desc sets are read by default.
            // Subpass inputs are read-only
            // Fragment outputs are write-only
            // Only storage image, storage texel and storage buffer are RW

            if (isWriteDescriptorType(resInfo.type)) {
                // If it is a write, it must wait for every read before it to finish
                for (const ResourceInfo& readResInfo : previousReads) {
                    // Need to add two dependencies here, make all reads 
                    // wait for the previous write, then make this 
                    // write wait for all reads. 
                    // If the last stages are only reads, add them at the end. 
                    VkSubpassDependency firstDependency{};
                    firstDependency.srcSubpass = readResInfo.pipelineIndex;
                    firstDependency.dstSubpass = resInfo.pipelineIndex;
                            
                    // Stage mask is obtained from resInfo's stage and param type 
                    // Mask just from type
                    firstDependency.srcStageMask = resInfoToVkStage(readResInfo);
                    firstDependency.srcAccessMask = decriptorTypeToAccessType[readResInfo.type];

                    firstDependency.dstStageMask = resInfoToVkStage(resInfo);
                    firstDependency.dstAccessMask = decriptorTypeToAccessType[resInfo.type];

                    VkSubpassDependency secondDependency{};
                    secondDependency.srcSubpass = lastWrite.pipelineIndex;
                    secondDependency.dstSubpass = readResInfo.pipelineIndex;
                            
                    // Stage mask is obtained from resInfo's stage and param type 
                    // Mask just from type
                    secondDependency.srcStageMask = resInfoToVkStage(lastWrite);
                    secondDependency.srcAccessMask = decriptorTypeToAccessType[lastWrite.type];

                    secondDependency.dstStageMask = resInfoToVkStage(readResInfo);
                    secondDependency.dstAccessMask = decriptorTypeToAccessType[readResInfo.type];

                    dependencies.push_back(firstDependency);
                    dependencies.push_back(secondDependency);   
                };

                previousReads.clear();
                lastWrite = resInfo;
            };
            // TODO: fragment output desc is the only write-only part, should that be treated differently?
            previousReads.push_back(resInfo);
        };
        // Now, if there are any reads leftover they must wait for the previous write. 
        for (const ResourceInfo& readResInfo : previousReads) {
            VkSubpassDependency dependency{};
            dependency.srcSubpass = lastWrite.pipelineIndex;
            dependency.dstSubpass = readResInfo.pipelineIndex;
                            
            dependency.srcStageMask = resInfoToVkStage(lastWrite);
            dependency.srcAccessMask = decriptorTypeToAccessType[lastWrite.type];

            dependency.dstStageMask = resInfoToVkStage(readResInfo);
            dependency.dstAccessMask = decriptorTypeToAccessType[readResInfo.type];

            dependencies.push_back(dependency); 
        };
    };

    void VulkanRenderPass::createDescriptorPool() {
        std::vector<VkDescriptorPoolSize> poolSizes{};

        for (DescriptorCounts::const_iterator it = _descriptorCounts.begin(); it != _descriptorCounts.end(); ++it) {
            VkDescriptorPoolSize poolSize{};
            poolSize.type = it->first;
            poolSize.descriptorCount = it->second;

            poolSizes.push_back(poolSize);
        };

        VkDescriptorPoolCreateInfo poolInfo{};
        poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
        poolInfo.pPoolSizes = poolSizes.data();
        poolInfo.maxSets = static_cast<uint32_t>(_pipelines.size()); // as many as pipelines, if we do 1 set per pipeline
        poolInfo.flags = 0;

        if (vkCreateDescriptorPool(_device, &poolInfo, nullptr, &_descriptorPool) != VK_SUCCESS) {
            throw std::runtime_error("Failed to create descriptor pool");
        }
    };

    VkDescriptorSetLayout VulkanRenderPass::createDescriptorSetLayout(const VulkanPipeline::VkUniforms& uniforms, const VulkanPipeline::VkSubpassInputs& subpassInputs) {
        VkDescriptorSetLayout layout;
        
        std::vector<VkDescriptorSetLayoutBinding> bindings;

        for (VulkanPipeline::VkUniforms::const_iterator it = uniforms.begin(); it != uniforms.end(); ++it) {
            
            VulkanPipeline::VkUniform uniform = it->second;
            // TODO: maybe move this inside the VkUniform when it's created?
            VkDescriptorSetLayoutBinding binding;
            binding.binding = uniform.decorationBinding;
            binding.descriptorType = uniform.descType;
            binding.descriptorCount = uniform.resourceCount;
            binding.stageFlags = uniform.stages;
            binding.pImmutableSamplers = nullptr; // TODO: allow immutable samplers

            bindings.push_back(binding);
        };
        
        // TODO: This is a bit of duplication, could fix with the thing from above maybe
        for (VulkanPipeline::VkSubpassInputs::const_iterator it = subpassInputs.begin(); it != subpassInputs.end(); ++it) {
            
            VulkanPipeline::VkSubpassInput subpassInput = it->second;
            
            VkDescriptorSetLayoutBinding binding;
            binding.binding = subpassInput.decorationBinding;
            binding.descriptorType = VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT;
            binding.descriptorCount = 1; // Fixed size 1 
            binding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT; // subpass inputs are fragment only 
            binding.pImmutableSamplers = nullptr; // TODO: as above

            bindings.push_back(binding);
        };

        VkDescriptorSetLayoutCreateInfo layoutInfo{};
        layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
        layoutInfo.pBindings = bindings.data();

        if (vkCreateDescriptorSetLayout(_device, &layoutInfo, nullptr, &layout) != VK_SUCCESS) {
            throw std::runtime_error("Failed to create descriptor set layout!");
        };

        return layout;
    };  

    VulkanRenderPass::VulkanPipelineInfo VulkanRenderPass::createVulkanPipelineInfo(const VulkanPipeline& pipeline, const uint8_t index) {
        VulkanPipelineInfo vkPipeline{};
        vkPipeline.descriptorSetLayout = createDescriptorSetLayout(pipeline._uniforms, pipeline._subpassInputs); // TODO: support only one layout for now 
        
        VkDescriptorSetAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        allocInfo.descriptorPool = _descriptorPool;
        allocInfo.descriptorSetCount = 1; // TODO: Max frames in flight thing here 
        allocInfo.pSetLayouts = &vkPipeline.descriptorSetLayout;

        if (vkAllocateDescriptorSets(_device, &allocInfo, &vkPipeline.descriptorSet) != VK_SUCCESS) {
            throw std::runtime_error("Failed to allocate descriptor set when creating VK pipelines!");
        };

        // TODO: Should this be moved to the VulkanPipeline object itself?
        // + duplication
        VkShaderModule vertShaderModule; 
        
        VkShaderModuleCreateInfo vertexCreateInfo{};
        vertexCreateInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
        vertexCreateInfo.codeSize = pipeline._spirvVertexShader.wordCount;
        vertexCreateInfo.pCode = pipeline._spirvVertexShader.spirvWords;

        if (vkCreateShaderModule(_device, &vertexCreateInfo, nullptr, &vertShaderModule) != VK_SUCCESS) {
            throw std::runtime_error("Failed to create vertex shader module!");
        };

        VkShaderModule fragShaderModule; 

        VkShaderModuleCreateInfo fragmentCreateInfo{};
        fragmentCreateInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
        fragmentCreateInfo.codeSize = pipeline._spirvVertexShader.wordCount;
        fragmentCreateInfo.pCode = pipeline._spirvVertexShader.spirvWords;

        if (vkCreateShaderModule(_device, &fragmentCreateInfo, nullptr, &fragShaderModule) != VK_SUCCESS) {
            throw std::runtime_error("Failed to create fragment shader module!");
        };

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

        VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
        vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
        vertexInputInfo.vertexBindingDescriptionCount = 1; // TODO: same as in binding desc function
        vertexInputInfo.pVertexBindingDescriptions = &pipeline._vertexInputBindingDesc;
        vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(pipeline._vertexInputAttributes.size());
        vertexInputInfo.pVertexAttributeDescriptions = pipeline._vertexInputAttributes.data();

        VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
        inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
        inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
        inputAssembly.primitiveRestartEnable = VK_FALSE;

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
        pipelineLayoutInfo.setLayoutCount = 1; // TODO: when would you have more than 1?
        pipelineLayoutInfo.pSetLayouts = &vkPipeline.descriptorSetLayout;
        pipelineLayoutInfo.pushConstantRangeCount = 0; // TODO: work push constants into the workflow
        pipelineLayoutInfo.pPushConstantRanges = nullptr;

        if (vkCreatePipelineLayout(_device, &pipelineLayoutInfo, nullptr, &vkPipeline.pipelineLayout) != VK_SUCCESS) {
            throw std::runtime_error("Failed to create Vulkan pipeline layout!");
        };

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
            throw std::runtime_error("Failed to create Vulkan graphics pipeline");
        };
        
        vkDestroyShaderModule(_device, fragShaderModule, nullptr);
        vkDestroyShaderModule(_device, vertShaderModule, nullptr);    

        return vkPipeline;        
    };

    // TODO: can descriptor writes be moved to the pipelines themselves actually?
    void VulkanRenderPass::writeDescriptorSet(const VkDescriptorSet descriptorSet, const VulkanPipeline::VkUniforms& uniforms, const VulkanPipeline::VkSubpassInputs& subpassInputs) {
        std::vector<VkWriteDescriptorSet> descriptorWrites{};

        for (VulkanPipeline::VkUniforms::const_iterator it = uniforms.begin(); it != uniforms.end(); ++it) {
                
            uint8_t arrayIndex = 0;
            for (const WdShaderResource& shaderResource : it->second.resources) {
                VkWriteDescriptorSet descriptorWrite{};
                    
                descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
                descriptorWrite.dstSet = descriptorSet; // TODO: can handle multiple frames in flight this way ?
                descriptorWrite.dstBinding = it->second.decorationBinding;
                descriptorWrite.dstArrayElement = arrayIndex; // set elements 1 by 1 
                descriptorWrite.descriptorType = it->second.descType;
                descriptorWrite.descriptorCount = 1; // static_cast<uint32_t>(uniform.resources.size());
                descriptorWrite.pBufferInfo = nullptr;
                descriptorWrite.pImageInfo = nullptr;
                descriptorWrite.pTexelBufferView = nullptr;

                if (isImageDescriptor(it->second.descType)) {
                    VkDescriptorImageInfo imageInfo{};
                    imageInfo.sampler = static_cast<VkSampler>(shaderResource.imageResource.sampler);
                    imageInfo.imageView = static_cast<VkImageView>(shaderResource.imageResource.image->target); // TODO:: write translate function for this to deal with default values
                    imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL; // TODO: In the spec it says this is the layout the image will be in when accessed from the descriptor. Do I have to do this transition manually?

                    descriptorWrite.pImageInfo = &imageInfo;
                } else if (isBufferDescriptor(it->second.descType)) {
                    if (it->second.descType == VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER || it->second.descType == VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER) {
                        descriptorWrite.pTexelBufferView = reinterpret_cast<const VkBufferView*>(&(shaderResource.bufferResource.bufferTarget)); // TODO: this might be a problem, might have to cast first to a variable then pass reference 
                    } else {
                        VkDescriptorBufferInfo bufferInfo{};
                        bufferInfo.buffer = static_cast<VkBuffer>(shaderResource.bufferResource.buffer->handle);
                        bufferInfo.offset = 0; //TODO: idk when it wouldn't be 
                        bufferInfo.range = static_cast<VkDeviceSize>(shaderResource.bufferResource.buffer->size); 

                        descriptorWrite.pBufferInfo = &bufferInfo;
                    };
                };

                descriptorWrites.push_back(descriptorWrite);
                arrayIndex++;
            };
        };

        for (VulkanPipeline::VkSubpassInputs::const_iterator it = subpassInputs.begin(); it != subpassInputs.end(); ++it) {
                
            VkDescriptorImageInfo imageInfo{};
            imageInfo.sampler = nullptr; // no sampler 
            imageInfo.imageView = static_cast<VkImageView>(it->second.resource->target); // TODO:: write translate function for this 
            imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;  // TODO: same as above

            VkWriteDescriptorSet descriptorWrite{};

            descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            descriptorWrite.dstSet = descriptorSet; // TODO: can handle multiple frames in flight this way ?
            descriptorWrite.dstBinding = it->second.decorationBinding;
            descriptorWrite.dstArrayElement = 0; // subpass inputs can't be in an array 
            descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT;
            descriptorWrite.descriptorCount = 1; // static_cast<uint32_t>(uniform.resources.size());
            descriptorWrite.pBufferInfo = nullptr;
            descriptorWrite.pImageInfo = &imageInfo;
            descriptorWrite.pTexelBufferView = nullptr;

            descriptorWrites.push_back(descriptorWrite);
        };
        
        vkUpdateDescriptorSets(_device, static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);
    };

    // Init functions 

    VulkanRenderPass::VulkanRenderPass(const std::vector<VulkanPipeline>& pipelines, VkDevice device, VkOffset2D renderOffset, VkExtent2D renderSize) : _pipelines(pipelines), _device(device) {
        _renderArea.offset = renderOffset;
        _renderArea.extent = renderSize;
    }; 
    
    void VulkanRenderPass::init() {
        // Create a map of ImageHanlde -> Attachment info (VkDesc, all the refs,
        // if the attachment will be used as present source)
        AttachmentMap attachments;
        uint8_t attachmentIndex = 0;
        // Create subpass descriptions for every subpass as we go along 
        std::vector<VkSubpassDescription> subpasses;

        // create subpass descriptions, attachment refs and framebuffer view array 
        // (pipeline <-> subpass being used interchangeably here)
        for (const VulkanPipeline& pipeline : _pipelines) {

            // resolve, depth, & preserve attachments, idk how to deal with yet 

            VulkanPipeline::VkSubpassInputs fragmentInputs = pipeline._subpassInputs;
            VulkanPipeline::VkFragmentOutputs fragmentOutputs = pipeline._fragmentOutputs;

            std::vector<VkAttachmentReference> inputRefs(fragmentInputs.size());
            std::vector<VkAttachmentReference> colorRefs(fragmentOutputs.size());

            updateSubpassInputAttachements(fragmentInputs, attachments, attachmentIndex, inputRefs, _framebuffer, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
            updateFragmentOutputAttachements(fragmentOutputs, attachments, attachmentIndex, colorRefs, _framebuffer, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
            
            VkSubpassDescription subpass{};
            subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;

            subpass.colorAttachmentCount = static_cast<uint32_t>(colorRefs.size());
            subpass.pColorAttachments = colorRefs.data();

            subpass.inputAttachmentCount = static_cast<uint32_t>(inputRefs.size());
            subpass.pInputAttachments = inputRefs.data();

            updateDepthStencilAttachment(pipeline._depthStencilResource, attachments, attachmentIndex, _framebuffer, subpass);

            // TODO: deal with preserve attachments (either here or later on once we understand all of them?)

            subpasses.push_back(subpass);
        };

        // At this point, I have a map of Handle -> AttachmentInfo. Each info has all the uses of an attachment via refs,
        // and everything should be correctly labeled and indexed. Can create the actual descriptions and dependencies now

        std::vector<VkAttachmentDescription> attachmentDescs(attachments.size());

        // TODO: Do I need manual layout transitions inbetween attachment usage?
        for (AttachmentMap::iterator it = attachments.begin(); it != attachments.end(); ++it) {
            // Need to check first and last ref. 
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
            };

            // TODO: we can specific here if memory of the attachemnt aliases, not sure when this arises 
            it->second.attachmentDesc.loadOp = loadOp;
            it->second.attachmentDesc.storeOp = storeOp;
            it->second.attachmentDesc.stencilLoadOp = loadOp; // For now, treat stencil components same as everything else. 
            it->second.attachmentDesc.stencilStoreOp = storeOp;
            it->second.attachmentDesc.initialLayout = initialLayout;
            it->second.attachmentDesc.finalLayout = finalLayout;

            attachmentDescs[info.attachmentIndex] = it->second.attachmentDesc;
        };
        
        // Need to generate all subpass dependencies now 

        ImageResources imageResources;
        BufferResources bufferResources;

        uint8_t pipelineIndex = 0;
        // Generate uniform resource maps now, for synchronisation 
        for (const VulkanPipeline& pipeline : _pipelines) {
            // Update uniform resource usage and also descriptor counts 
            updateUniformResources(pipeline._uniforms, imageResources, bufferResources, pipelineIndex, _descriptorCounts);

            // Fragment-only subpass inputs and outs
            // TODO : repetetition of loop code with above ref code ^
            VulkanPipeline::VkSubpassInputs fragmentInputs = pipeline._subpassInputs;
            VulkanPipeline::VkFragmentOutputs fragmentOutputs = pipeline._fragmentOutputs;

            updateAttachmentResources<VulkanPipeline::VkSubpassInputs>(fragmentInputs, imageResources, VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, pipelineIndex);
            updateAttachmentResources<VulkanPipeline::VkFragmentOutputs>(fragmentOutputs, imageResources, FRAGMENT_OUTPUT_DESC, pipelineIndex);
            // TODO: depth should be handled here too.
            pipelineIndex++;
        };

        std::vector<VkSubpassDependency> dependencies;

        for (ImageResources::iterator it = imageResources.begin(); it != imageResources.end(); ++it) {
            addDependencies(dependencies, it->second);
        };

        for (BufferResources::iterator it = bufferResources.begin(); it != bufferResources.end(); ++it) {
            addDependencies(dependencies, it->second);
        };
        // All dependencies except external should be added now.

        // Can create the actual render pass object now 
        VkRenderPassCreateInfo renderPassInfo{};
        renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
        renderPassInfo.attachmentCount = static_cast<uint32_t>(attachmentDescs.size());
        renderPassInfo.pAttachments = attachmentDescs.data();
        renderPassInfo.subpassCount = static_cast<uint32_t>(subpasses.size());
        renderPassInfo.pSubpasses = subpasses.data();
        renderPassInfo.dependencyCount = static_cast<uint32_t>(dependencies.size());
        renderPassInfo.pDependencies = dependencies.data();

        if (vkCreateRenderPass(_device, &renderPassInfo, nullptr, &_renderPass) != VK_SUCCESS) {
            throw std::runtime_error("Failed to create Vulkan render pass!");
        };

        // Render pass has been created, can create framebuffer now.

        VkFramebufferCreateInfo framebufferInfo{};
        framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebufferInfo.renderPass = _renderPass;
        framebufferInfo.attachmentCount = static_cast<uint32_t>(_framebuffer.attachmentViews.size());
        framebufferInfo.pAttachments = _framebuffer.attachmentViews.data();
        framebufferInfo.width = _renderArea.extent.width; // TODO: By default I'm mapping the render area to the framebuffer, don't know if this is desirable or not yet
        framebufferInfo.height = _renderArea.extent.height;
        framebufferInfo.layers = 1;

        if (vkCreateFramebuffer(_device, &framebufferInfo, nullptr, &_framebuffer.framebuffer) != VK_SUCCESS) {
            throw std::runtime_error("Failed to create Vulkan framebuffers");
        };

        createDescriptorPool(); // Create descriptor pool first, then allocate the sets and create writes when making the pipeline objects

        // Create all Vulkan pipelines & descriptor writes now 
        uint8_t index = 0;
        for (const VulkanPipeline& pipeline : _pipelines) {
            VulkanPipelineInfo vkPipelineInfo = createVulkanPipelineInfo(pipeline, index);
            _vkPipelines.push_back(vkPipelineInfo);

            writeDescriptorSet(vkPipelineInfo.descriptorSet, pipeline._uniforms, pipeline._subpassInputs);
            index++; 
        };
    };
};