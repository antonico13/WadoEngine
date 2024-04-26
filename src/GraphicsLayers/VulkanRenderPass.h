#ifndef WADO_VULKAN_RENDER_PASS
#define WADO_VULKAN_RENDER_PASS

#include "vulkan/vulkan.h"

#include "WdImage.h"
#include "WdPipeline.h"
#include "WdRenderPass.h"

#include "VulkanLayer.h"
#include "VulkanPipeline.h"

#include <map>
#include <tuple>

namespace Wado::GAL::Vulkan { 
    // Forward declarations
    class VulkanLayer;
    class VulkanCommandList;
    class VulkanPipeline;

    class VulkanRenderPass : public WdRenderPass {
        public:
            friend class VulkanLayer;
            friend class VulkanCommandList;

        private:
            using VulkanPipelineInfo = struct VulkanPipelineInfo {
                VkPipeline pipeline;
                // Have vector of descriptors per frame in-flight. 
                // Index by frame in flight, then by set number 
                std::vector<std::vector<VkDescriptorSet>> descriptorSets;
            };

            using AttachmentInfo = struct AttachmentInfo {
                VkAttachmentDescription attachmentDesc;            
                std::vector<VkAttachmentReference> refs;
                uint8_t attachmentIndex;
                bool presentSrc;
            };

            using AttachmentMap = std::map<WdResourceID, AttachmentInfo>;

            using ResourceInfo = struct ResourceInfo {
                VkDescriptorType type;
                VkShaderStageFlags stages;
                VkPipelineStageFlags pipelineStages;
                uint8_t pipelineIndex;
            };

            using ResourceMap = std::map<WdResourceID, std::vector<ResourceInfo>>;

            using Framebuffer = struct Framebuffer {
                std::vector<VkImageView> attachmentViews;
                std::vector<VkClearValue> clearValues; // TODO: should this be per big framebuffer? Since the clear values should always be the same between frames 
                VkFramebuffer framebuffer;
            };

            using RenderArea = struct RenderArea {
                VkOffset2D offset;
                VkExtent2D extent;
            };

            using DescriptorCounts = std::map<VkDescriptorType, uint32_t>;

            static const VkDescriptorType FRAGMENT_OUTPUT_DESC = VkDescriptorType::VK_DESCRIPTOR_TYPE_MAX_ENUM; 
            static const VkDescriptorType DEPTH_STENCIL_DESC = VkDescriptorType::VK_DESCRIPTOR_TYPE_MUTABLE_VALVE; // TODO: this is very not good, not sure how to extend desc types in a better way...

            static bool isWriteDescriptorType(VkDescriptorType type);
            static VkPipelineStageFlags VkShaderStageFlagsToVkPipelineStageFlags(const VkShaderStageFlags stageFlags);

            static std::map<VkDescriptorType, VkAccessFlags> decriptorTypeToAccessType;

            static void updateUniformResources(const VulkanPipeline::VkSetUniforms& uniforms, ResourceMap& resourceMap, const uint8_t pipelineIndex, DescriptorCounts& descriptorCounts);
            
            template <class T>
            static void updateAttachmentResources(const T& attachments, AttachmentMap& attachmentMap, ResourceMap& resourceMap, std::vector<VkAttachmentReference>& attachmentRefs, std::vector<Framebuffer>& framebuffers, const VkImageLayout layout, const VkDescriptorType type, const uint8_t pipelineIndex);
            static void updateDepthStencilAttachmentResource(const Memory::WdClonePtr<WdImage> depthStencilAttachment, AttachmentMap& attachments, ResourceMap& resourceMap, std::vector<Framebuffer>& framebuffers, const VkDescriptorType type, const uint8_t pipelineIndex, VkSubpassDescription& subpass);

            static void addDependencies(std::vector<VkSubpassDependency>& dependencies, const std::vector<ResourceInfo>& resInfos);

            void createDescriptorPool();
            
            VulkanPipelineInfo createVulkanPipelineInfo(const VulkanPipeline& pipeline, const uint8_t index);

            VulkanRenderPass(const std::vector<VulkanPipeline>& pipelines, VkDevice device, VkOffset2D renderOffset, VkExtent2D renderSize);
            
            void init();

            VkDevice _device;
            VkRenderPass _renderPass;
            std::vector<Framebuffer> _framebuffers; // per frame in flight/swap chain image count 
            RenderArea _renderArea;
            VkDescriptorPool _descriptorPool;
            DescriptorCounts _descriptorCounts;
            
            const std::vector<VulkanPipeline>& _pipelines;
            std::vector<VulkanPipelineInfo> _vkPipelines;
    };
};

#endif