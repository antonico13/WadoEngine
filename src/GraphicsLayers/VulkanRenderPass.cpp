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

    // const VkDescriptorType VulkanRenderPass::WdShaderParamTypeToVkDescriptorType[] = {
    //             VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
    //             VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE,
    //             VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
    //             VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER,
    //             VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER,
    //             VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
    //             VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT,
    //             VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
    //             VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, // this is temp only 
    //             VK_DESCRIPTOR_TYPE_SAMPLER,
    // };

    // // Static helper functions

    // VkShaderStageFlagBits VulkanRenderPass::WdStagesToVkStages(const WdStageMask stages) {

    // };

    // VkPipelineStageFlags VulkanRenderPass::ResInfoToVkStage(const ResourceInfo& resInfo) {

    // };

    // void VulkanRenderPass::addDependencies(std::vector<VkSubpassDependency>& dependencies, std::vector<ResourceInfo>& resInfos, uint32_t readMask, uint32_t writeMask) {

    // };

    // void VulkanRenderPass::updateUniformResources(const WdPipeline::WdUniforms& resourceMap, ImageResources& imageResources, BufferResources& bufferResources, uint8_t pipelineIndex) {

    // };
            
    // template <class T>
    // void VulkanRenderPass::updateAttachmentResources(const T& resourceMap, ImageResources& imageResources, uint8_t pipelineIndex) {

    // };
            
    // VertexInputDesc VulkanRenderPass::createVertexAttributeDescriptionsAndBinding(const WdPipeline::WdVertexInputs& vertexInputs) {

    // };

    // void VulkanRenderPass::addDescriptorPoolSizes(const WdPipeline& pipeline, std::vector<VkDescriptorPoolSize>& poolSizes) {

    // };

    // // Instance helper functions 

    // VkDescriptorSetLayout VulkanRenderPass::createDescriptorSetLayout(const WdPipeline::WdUniforms& uniforms, const WdPipeline::WdSubpassInputs& subpassInputs) {

    // };

    // void VulkanRenderPass::writeDescriptorSets(const std::vector<VkDescriptorSet>& descriptorSets, const WdPipeline::WdUniforms& uniforms, const WdPipeline::WdSubpassInputs& subpassInputs) {
        
    // };

    // void VulkanRenderPass::createDescriptorPool() {

    // };

    // VulkanPipeline VulkanRenderPass::createVulkanPipeline(const WdPipeline& pipeline, uint8_t index) {

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

    void VulkanRenderPass::updateFragmentOutputAttachements(const VulkanPipeline::VkFragmentOutputs& resourceMap, AttachmentMap& attachments, uint8_t& attachmentIndex, std::vector<VkAttachmentReference>& attachmentRefs, std::vector<VkImageView>& framebuffer, VkImageLayout layout) {
        for (VulkanPipeline::VkFragmentOutputs::const_iterator it = resourceMap.begin(); it != resourceMap.end(); ++it) {
            
            WdImageHandle attachmentHandle = it->second.resource->handle;
            uint8_t attachmentArrayIndex = it->second.decorationLocation;

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
                framebuffer.push_back(static_cast<VkImageView>(it->second.resource->target));
            };

            // Create attachment ref 
            VkAttachmentReference attachmentRef{};
            attachmentRef.layout = layout; 
            attachmentRef.attachment = attachments[attachmentHandle].attachmentIndex;

            attachments[attachmentHandle].refs.push_back(attachmentRef);

            attachmentRefs[attachmentArrayIndex] = attachmentRef;
        }; 
    };

    void VulkanRenderPass::updateSubpassInputAttachements(const VulkanPipeline::VkSubpassInputs& resourceMap, AttachmentMap& attachments, uint8_t& attachmentIndex, std::vector<VkAttachmentReference>& attachmentRefs, std::vector<VkImageView>& framebuffer, VkImageLayout layout) {
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
                framebuffer.push_back(static_cast<VkImageView>(it->second.resource->target));
            };

            // Create attachment ref 
            VkAttachmentReference attachmentRef{};
            attachmentRef.layout = layout; 
            attachmentRef.attachment = attachments[attachmentHandle].attachmentIndex;

            attachments[attachmentHandle].refs.push_back(attachmentRef);

            attachmentRefs[attachmentArrayIndex] = attachmentRef;
        }; 
    };

    // Init functions 

    VulkanRenderPass::VulkanRenderPass(const std::vector<VulkanPipeline>& pipelines, VkDevice device) : _pipelines(pipelines), _device(device) {}; 
    
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

            updateAttachements<VulkanPipeline::VkSubpassInputs>(fragmentInputs, attachments, attachmentIndex, inputRefs, _framebuffer.attachmentViews, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
            updateAttachements<VulkanPipeline::VkFragmentOutputs>(fragmentOutputs, attachments, attachmentIndex, colorRefs, _framebuffer.attachmentViews, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
            
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
};