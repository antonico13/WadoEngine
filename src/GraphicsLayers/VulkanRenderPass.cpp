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
    // TODO: a bit of repetition in all the update functions 
    void VulkanRenderPass::updateFragmentOutputAttachements(const VulkanPipeline::VkFragmentOutputs& resourceMap, AttachmentMap& attachments, uint8_t& attachmentIndex, std::vector<VkAttachmentReference>& attachmentRefs, std::vector<VkImageView>& framebuffer, VkImageLayout layout) {
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

    void VulkanRenderPass::updateDepthStencilAttachment(const Memory::WdClonePtr<WdImage> depthStencilAttachment, AttachmentMap& attachments, uint8_t& attachmentIndex, std::vector<VkImageView>& framebuffer, VkSubpassDescription& subpass) {
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
            framebuffer.push_back(static_cast<VkImageView>(depthStencilAttachment->target));
        };

        // Create attachment ref 
        VkAttachmentReference attachmentRef{};
        attachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL; 
        attachmentRef.attachment = attachments[attachmentHandle].attachmentIndex;

        attachments[attachmentHandle].refs.push_back(attachmentRef);

        // Update subpass description now.
        subpass.pDepthStencilAttachment = &attachmentRef; // TODO: I'm pretty sure the ref dies? 
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

            updateSubpassInputAttachements(fragmentInputs, attachments, attachmentIndex, inputRefs, _framebuffer.attachmentViews, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
            updateFragmentOutputAttachements(fragmentOutputs, attachments, attachmentIndex, colorRefs, _framebuffer.attachmentViews, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
            
            VkSubpassDescription subpass{};
            subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;

            subpass.colorAttachmentCount = static_cast<uint32_t>(colorRefs.size());
            subpass.pColorAttachments = colorRefs.data();

            subpass.inputAttachmentCount = static_cast<uint32_t>(inputRefs.size());
            subpass.pInputAttachments = inputRefs.data();

            updateDepthStencilAttachment(pipeline._depthStencilResource, attachments, attachmentIndex, _framebuffer.attachmentViews, subpass);

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
        
        // need to generate all deps now 

        ImageResources imageResources;
        BufferResources bufferResources;

        uint8_t pipelineIndex = 0;
        // Generate uniform resource maps now, for synchronisation 
        for (const VulkanPipeline& pipeline : _pipelines) {
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