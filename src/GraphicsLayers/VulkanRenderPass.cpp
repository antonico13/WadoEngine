#include "VulkanRenderPass.h"

namespace Wado::GAL::Vulkan {

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

    VkPipelineStageFlags VulkanRenderPass::VkShaderStageFlagsToVkPipelineStageFlags(const VkShaderStageFlags stageFlags) {
        VkPipelineStageFlags pipelineStages = 0;
        
        if (stageFlags & VK_SHADER_STAGE_VERTEX_BIT) {
            pipelineStages |= VK_PIPELINE_STAGE_VERTEX_SHADER_BIT;
        };

        if (stageFlags & VK_SHADER_STAGE_FRAGMENT_BIT) {
            pipelineStages |= VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
        };

        return pipelineStages;
    };
// END of utils 
    template <class T>
    void VulkanRenderPass::updateAttachmentResources(const T& attachments, AttachmentMap& attachmentMap, ResourceMap& resourceMap, std::vector<VkAttachmentReference>& attachmentRefs, std::vector<Framebuffer>& framebuffers, const VkImageLayout layout, const VkDescriptorType type, const uint8_t pipelineIndex) {
        for (T::const_iterator it = attachments.begin(); it != attachments.end(); ++it) {

            WdResourceID attachmentID = it->second.resource->resourceID;
            
            // This defines where the attachment ref should be placed 
            // in pColorAttachments, pInputAttachments, etc.
            uint8_t attachmentRefArrayIndex = it->second.decorationLocation; 

            AttachmentMap::iterator attachment = attachmentMap.find(attachmentID);
            
            if (attachment == attachmentMap.end()) {
                // First time seeing this resource used as an attachment, need to create new entry
                VkAttachmentDescription attachmentDesc{};
                attachmentDesc.format = WdFormatToVkFormat[it->second.resource->format];
                attachmentDesc.samples = VK_SAMPLE_COUNT_1_BIT; // TODO :: Figure out how to deal with samples. 
                // layouts, store and load ops are calculated later

                AttachmentInfo info{}; 
                info.attachmentIndex = attachmentMap.size(); // as we add attachments, index increases 1 by 1 
                info.attachmentDesc = attachmentDesc;
                info.presentSrc = it->second.resource->usage & WdImageUsage::WD_PRESENT;
                            
                // Add info to attachment map and framebuffers
                attachmentMap[attachmentID] = info; 
                // Framebuffer location maps to attachment index 
                for (int i = 0; i < framebuffers.size(); i++) {
                    framebuffers[i].attachmentViews.push_back(static_cast<VkImageView>(it->second.resource->targets[i])); // This has to be per-frame/swapchain index 
                    framebuffers[i].clearValues.push_back(WdClearValueToVkClearValue(it->second.resource->clearValue));
                };
            };

            // Create attachment ref 
            VkAttachmentReference attachmentRef{};
            attachmentRef.layout = layout; 
            attachmentRef.attachment = attachmentMap[attachmentID].attachmentIndex;

            attachmentMap[attachmentID].refs.push_back(attachmentRef);

            attachmentRefs[attachmentRefArrayIndex] = attachmentRef; 

            // Need to add the resource information for this attachment now 

            ResourceInfo resInfo{}; 
            resInfo.type = type;
            resInfo.stages = VK_SHADER_STAGE_FRAGMENT_BIT; // all attachments will be used in the fragment stage 
            resInfo.pipelineIndex = pipelineIndex; 
            resInfo.pipelineStages = type == VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT ? VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT : VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT; // TODO: Janky, needs fixing
            
            ResourceMap::iterator resIter = resourceMap.find(attachmentID); 
            
            if (resIter == resourceMap.end()) { 
                // First ever use of this resource
                std::vector<ResourceInfo> resInfos; 
                resourceMap[attachmentID] = resInfos; 
            };

            resourceMap[attachmentID].push_back(resInfo); 
        };  
    };

    // TODO : move descriptor counts to pipeline?
    void VulkanRenderPass::updateUniformResources(const VulkanPipeline::VkSetUniforms& uniforms, ResourceMap& resourceMap, const uint8_t pipelineIndex, DescriptorCounts& descriptorCounts) {
        for (VulkanPipeline::VkSetUniforms::const_iterator it = uniforms.begin(); it != uniforms.end(); ++it) {
            VkDescriptorType descType = it->second.binding.descriptorType;
            // We are checking every uniform in a pipeline here, so it's worth also updating descriptor counts
            // for pool creation later on.

            if (descriptorCounts.find(descType) == descriptorCounts.end()) {
                descriptorCounts[descType] = 0;
            };
            descriptorCounts[descType] += it->second.binding.descriptorCount;
            
            // This is analogous to the attachment ref earlier      
            ResourceInfo resInfo{}; 
            resInfo.type = descType; 
            resInfo.pipelineIndex = pipelineIndex; 
            resInfo.stages = it->second.binding.stageFlags; // TODO: Is this right? Do I even care about this?
            resInfo.pipelineStages = VkShaderStageFlagsToVkPipelineStageFlags(it->second.binding.stageFlags);

            for (const WdResourceID& resourceID : it->second.resourceIDs) {
                ResourceMap::iterator resIter = resourceMap.find(resourceID); 
                
                if (resIter == resourceMap.end()) { 
                    // First ever use of this resource
                    std::vector<ResourceInfo> resInfos; 
                    resourceMap[resourceID] = resInfos; 
                };
                resourceMap[resourceID].push_back(resInfo); 
            };
        };   
    };

    void VulkanRenderPass::updateDepthStencilAttachmentResource(const Memory::WdClonePtr<WdImage> depthStencilAttachment, AttachmentMap& attachmentMap, ResourceMap& resourceMap, std::vector<Framebuffer>& framebuffers, const VkDescriptorType type, const uint8_t pipelineIndex, VkSubpassDescription& subpass) {
        if (!depthStencilAttachment) {
            return; // Depth stencil not set 
        };

        // If not, look if this image has been used before
        WdResourceID attachmentID = depthStencilAttachment->resourceID;
        AttachmentMap::iterator attachment = attachmentMap.find(attachmentID);

        if (attachment == attachmentMap.end()) { // Not found, needs desc.
            // First time seeing this resource used as an attachment, need to create new entry
            VkAttachmentDescription attachmentDesc{};
            attachmentDesc.format = WdFormatToVkFormat[depthStencilAttachment->format];
            attachmentDesc.samples = VK_SAMPLE_COUNT_1_BIT; // TODO :: Figure out how to deal with samples. 
            // layouts, store and load ops are calculated later

            AttachmentInfo info{}; 
            info.attachmentIndex = attachmentMap.size();
            info.attachmentDesc = attachmentDesc;
            info.presentSrc = depthStencilAttachment->usage & WdImageUsage::WD_PRESENT;
                                
            // Add info to attachment map and framebuffers
            attachmentMap[attachmentID] = info;
            // Framebuffer location maps to attachment index  
            for (int i = 0; i < framebuffers.size(); i++) {
                framebuffers[i].attachmentViews.push_back(static_cast<VkImageView>(depthStencilAttachment->targets[i])); // This has to be per-frame/swapchain index 
                framebuffers[i].clearValues.push_back(WdClearValueToVkClearValue(depthStencilAttachment->clearValue));
            };
        };

        // Create attachment ref 
        VkAttachmentReference attachmentRef{};
        attachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL; 
        attachmentRef.attachment = attachmentMap[attachmentID].attachmentIndex;

        attachmentMap[attachmentID].refs.push_back(attachmentRef);

        // Update subpass description now.
        subpass.pDepthStencilAttachment = &attachmentRef; // TODO: I'm pretty sure the ref dies? 

        ResourceInfo resInfo{}; 
        resInfo.type = type;
        resInfo.stages = VK_SHADER_STAGE_FRAGMENT_BIT; 
        resInfo.pipelineStages = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
        resInfo.pipelineIndex = pipelineIndex; 
            
        ResourceMap::iterator resInfoIter = resourceMap.find(attachmentID); 
            
        if (resInfoIter == resourceMap.end()) { 
            // not found, init array
            std::vector<ResourceInfo> resInfos; 
            resourceMap[attachmentID] = resInfos; 
        };

        resourceMap[attachmentID].push_back(resInfo); 
    };


    // TODO: should util functions all be moved to VkLayer?
    bool VulkanRenderPass::isWriteDescriptorType(VkDescriptorType type) {
        return type == VK_DESCRIPTOR_TYPE_STORAGE_IMAGE ||
            type == VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER || 
            type == VK_DESCRIPTOR_TYPE_STORAGE_BUFFER ||
            type == VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC ||
            type == FRAGMENT_OUTPUT_DESC || type == DEPTH_STENCIL_DESC;
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
            {DEPTH_STENCIL_DESC, VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT},
        };

    // need to refactor the duplication here 
    void VulkanRenderPass::addDependencies(std::vector<VkSubpassDependency>& dependencies, const std::vector<ResourceInfo>& resInfos) {
        std::vector<ResourceInfo> previousReads{};
        // TODO: This effectively adds an external dep on the color attachments, but this is for every 
        // resource. I don't think this will work.
        ResourceInfo lastWrite{};
        lastWrite.stages = VkShaderStageFlagBits::VK_SHADER_STAGE_ALL; // TODO: Should I change it so every time I use enums I mention the scope?
        lastWrite.type = FRAGMENT_OUTPUT_DESC;
        lastWrite.pipelineStages = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        lastWrite.pipelineIndex = VK_SUBPASS_EXTERNAL;
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
                    firstDependency.srcStageMask = readResInfo.pipelineStages;
                    firstDependency.srcAccessMask = decriptorTypeToAccessType[readResInfo.type];

                    firstDependency.dstStageMask = resInfo.pipelineStages;
                    firstDependency.dstAccessMask = decriptorTypeToAccessType[resInfo.type];

                    VkSubpassDependency secondDependency{};
                    secondDependency.srcSubpass = lastWrite.pipelineIndex;
                    secondDependency.dstSubpass = readResInfo.pipelineIndex;
                            
                    // Stage mask is obtained from resInfo's stage and param type 
                    // Mask just from type
                    secondDependency.srcStageMask = lastWrite.pipelineStages;
                    secondDependency.srcAccessMask = decriptorTypeToAccessType[lastWrite.type];

                    secondDependency.dstStageMask = readResInfo.pipelineStages;
                    secondDependency.dstAccessMask = decriptorTypeToAccessType[readResInfo.type];

                    dependencies.push_back(firstDependency);
                    dependencies.push_back(secondDependency);   
                };

                previousReads.clear();
                lastWrite = resInfo;
            };
            // TODO: fragment output desc is the only write-only part, should that be treated differently?
            // TODO: for RW things this becomes a self dependency, I think that's incorrect in Vulkan
            previousReads.push_back(resInfo);
        };
        // Now, if there are any reads leftover they must wait for the previous write. 
        for (const ResourceInfo& readResInfo : previousReads) {
            VkSubpassDependency dependency{};
            dependency.srcSubpass = lastWrite.pipelineIndex;
            dependency.dstSubpass = readResInfo.pipelineIndex;
                            
            dependency.srcStageMask = lastWrite.pipelineStages;
            dependency.srcAccessMask = decriptorTypeToAccessType[lastWrite.type];

            dependency.dstStageMask = readResInfo.pipelineStages;
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

        uint32_t maxDescriptorSetCount = 0;

        for (int i = 0; i < _pipelines.size(); i++) {
            maxDescriptorSetCount += _pipelines[i]._descriptorSetBindings.size(); // this is effectively number of sets 
        };

        maxDescriptorSetCount *= _framebuffers.size(); // as many framebuffers as max frames in flight 

        VkDescriptorPoolCreateInfo poolInfo{};
        poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
        poolInfo.pPoolSizes = poolSizes.data();
        poolInfo.maxSets = maxDescriptorSetCount; // number of sets that can be allocated is: (total number of sets per pipeline) * total frames in flight at most 
        poolInfo.flags = 0;

        if (vkCreateDescriptorPool(_device, &poolInfo, nullptr, &_descriptorPool) != VK_SUCCESS) {
            throw std::runtime_error("Failed to create descriptor pool");
        };
    };

    VulkanRenderPass::VulkanPipelineInfo VulkanRenderPass::createVulkanPipelineInfo(const VulkanPipeline& pipeline, const uint8_t index) {
        VulkanPipelineInfo vkPipeline{};

        vkPipeline.descriptorSets.resize(_framebuffers.size());
        
        // for each frame in flight, allocate all descriptor sets
        for (int i = 0; i < _framebuffers.size(); i++) {
            
            vkPipeline.descriptorSets[i].resize(pipeline._descriptorSetLayouts.size());

            VkDescriptorSetAllocateInfo allocInfo{};
            allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
            allocInfo.descriptorPool = _descriptorPool;
            allocInfo.descriptorSetCount = static_cast<uint32_t>(pipeline._descriptorSetLayouts.size());
            allocInfo.pSetLayouts = pipeline._descriptorSetLayouts.data();

            if (vkAllocateDescriptorSets(_device, &allocInfo, vkPipeline.descriptorSets[i].data()) != VK_SUCCESS) {
                throw std::runtime_error("Failed to allocate descriptor set when creating Vulkan pipelines in Vulkan Render Pass!");
            };   
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

        // TODO: do this based on whether the depth resource is set or not 
        VkPipelineDepthStencilStateCreateInfo depthStencil{};
        depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
        depthStencil.depthTestEnable = pipeline._depthStencilResource ? VK_TRUE : VK_FALSE;
        depthStencil.depthWriteEnable = pipeline._depthStencilResource ? VK_TRUE : VK_FALSE;
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
        pipelineInfo.layout = pipeline._pipelineLayout;
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

    // Init functions 

    VulkanRenderPass::VulkanRenderPass(const std::vector<VulkanPipeline>& pipelines, VkDevice device, VkOffset2D renderOffset, VkExtent2D renderSize) : _pipelines(pipelines), _device(device) {
        _renderArea.offset = renderOffset;
        _renderArea.extent = renderSize;
    }; 
    
    void VulkanRenderPass::init() {
        // Create a map of WdResourceID -> Attachment info (VkDesc, all the refs,
        // if the attachment will be used as present source)
        AttachmentMap attachments;
        // The resource map records every use of a resource (WdResourceID -> ResInfo),
        // such that subpass dependencies can be created 
        ResourceMap resources;
        // Create subpass descriptions for every subpass as we go along 
        std::vector<VkSubpassDescription> subpasses;

        // create subpass descriptions, attachment refs and framebuffer view array 
        // (pipeline <-> subpass being used interchangeably here)
        for (int i = 0; i < _pipelines.size(); i++) {

            // resolve, depth, & preserve attachments, idk how to deal with yet 

            VulkanPipeline::VkSubpassInputs fragmentInputs = _pipelines[i]._subpassInputs;
            VulkanPipeline::VkFragmentOutputs fragmentOutputs = _pipelines[i]._fragmentOutputs;

            std::vector<VkAttachmentReference> inputRefs(fragmentInputs.size());
            std::vector<VkAttachmentReference> colorRefs(fragmentOutputs.size());
            
            updateAttachmentResources<VulkanPipeline::VkFragmentOutputs>(fragmentOutputs, attachments, resources, colorRefs, _framebuffers, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, FRAGMENT_OUTPUT_DESC, i);
            updateAttachmentResources<VulkanPipeline::VkSubpassInputs>(fragmentInputs, attachments, resources, inputRefs, _framebuffers, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, i);

            for (const VulkanPipeline::VkSetUniforms& vkSetUniforms : _pipelines[i]._uniforms) {
                // TODO: should descriptor counts be in the pipelines too?
                updateUniformResources(vkSetUniforms, resources, i, _descriptorCounts);
            };

            VkSubpassDescription subpass{};
            subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;

            subpass.colorAttachmentCount = static_cast<uint32_t>(colorRefs.size());
            subpass.pColorAttachments = colorRefs.data();

            subpass.inputAttachmentCount = static_cast<uint32_t>(inputRefs.size());
            subpass.pInputAttachments = inputRefs.data();
            
            updateDepthStencilAttachmentResource(_pipelines[i]._depthStencilResource, attachments, resources, _framebuffers, DEPTH_STENCIL_DESC, i, subpass);

            // TODO: deal with preserve & resolve attachments (either here or later on once we understand all of them?)

            subpasses.push_back(subpass);
        };

        // At this point, I have a map of ResourceID -> AttachmentInfo. Each info has all the uses of an attachment via refs,
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

            // TODO: I can specify here if memory of the attachemnt aliases, not sure when this arises 
            it->second.attachmentDesc.loadOp = loadOp;
            it->second.attachmentDesc.storeOp = storeOp;
            it->second.attachmentDesc.stencilLoadOp = loadOp; // For now, treat stencil components same as everything else. 
            it->second.attachmentDesc.stencilStoreOp = storeOp;
            it->second.attachmentDesc.initialLayout = initialLayout;
            it->second.attachmentDesc.finalLayout = finalLayout;

            attachmentDescs[info.attachmentIndex] = it->second.attachmentDesc;
        };

        std::vector<VkSubpassDependency> dependencies;

        for (ResourceMap::const_iterator it = resources.begin(); it != resources.end(); ++it) {
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

        // Render pass has been created, can create framebuffers now, these SHOULD BE per-swap chain image

        for (int i = 0; i < _framebuffers.size(); i ++) {
            VkFramebufferCreateInfo framebufferInfo{};
            framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
            framebufferInfo.renderPass = _renderPass;
            framebufferInfo.attachmentCount = static_cast<uint32_t>(_framebuffers[i].attachmentViews.size());
            framebufferInfo.pAttachments = _framebuffers[i].attachmentViews.data();
            framebufferInfo.width = _renderArea.extent.width; // TODO: By default I'm mapping the render area to the framebuffer, don't know if this is desirable or not yet
            framebufferInfo.height = _renderArea.extent.height;
            framebufferInfo.layers = 1;

            if (vkCreateFramebuffer(_device, &framebufferInfo, nullptr, &_framebuffers[i].framebuffer) != VK_SUCCESS) {
                throw std::runtime_error("Failed to create Vulkan framebuffers");
            };
        };

        createDescriptorPool(); // Create descriptor pool first, then allocate the sets and create writes when making the pipeline objects

        // Create all Vulkan pipelines & allocate descriptor sets now 
        for (int i = 0; i < _pipelines.size(); i++) {
            VulkanPipelineInfo vkPipelineInfo = createVulkanPipelineInfo(_pipelines[i], i);
            _vkPipelines.push_back(vkPipelineInfo);
        };
    };
};