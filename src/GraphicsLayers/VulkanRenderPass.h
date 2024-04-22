#ifndef WADO_VULKAN_RENDER_PASS
#define WADO_VULKAN_RENDER_PASS

#include "vulkan/vulkan.h"

#include "WdImage.h"
#include "WdPipeline.h"
#include "WdRenderPass.h"

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
                VkPipelineLayout pipelineLayout;
                VkDescriptorSetLayout descriptorSetLayout;
                VkDescriptorSet descriptorSet;
            };

            using AttachmentInfo = struct AttachmentInfo {
                VkAttachmentDescription attachmentDesc;            
                std::vector<VkAttachmentReference> refs;
                uint8_t attachmentIndex;
                bool presentSrc;
            };

            using AttachmentMap = std::map<WdImageHandle, AttachmentInfo>;

            using ResourceInfo = struct ResourceInfo {
                VkDescriptorType type;
                VkShaderStageFlags stages;
                uint8_t pipelineIndex;
            };

            using ImageResources = std::map<WdImageHandle, std::vector<ResourceInfo>>;
            using BufferResources = std::map<WdBufferHandle, std::vector<ResourceInfo>>;

            using Framebuffer = struct Framebuffer {
                std::vector<VkImageView> attachmentViews;
                std::vector<VkClearValue> clearValues;
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
            static VkPipelineStageFlags resInfoToVkStage(const ResourceInfo& resInfo);

            static std::map<VkDescriptorType, VkAccessFlags> decriptorTypeToAccessType;

            static void updateFragmentOutputAttachements(const VulkanPipeline::VkFragmentOutputs& resourceMap, AttachmentMap& attachments, uint8_t& attachmentIndex, std::vector<VkAttachmentReference>& attachmentRefs, Framebuffer& framebuffer, VkImageLayout layout);
            static void updateSubpassInputAttachements(const VulkanPipeline::VkSubpassInputs& resourceMap, AttachmentMap& attachments, uint8_t& attachmentIndex, std::vector<VkAttachmentReference>& attachmentRefs, Framebuffer& framebuffer, VkImageLayout layout);
            static void updateDepthStencilAttachment(const Memory::WdClonePtr<WdImage> depthStencilAttachment, AttachmentMap& attachments, uint8_t& attachmentIndex, Framebuffer& framebuffer, VkSubpassDescription& subpass);
            static void updateUniformResources(const VulkanPipeline::VkUniforms& resourceMap, ImageResources& imageResources, BufferResources& bufferResources, const uint8_t pipelineIndex, DescriptorCounts& descriptorCounts);
            template <class T>
            static void updateAttachmentResources(const T& resourceMap, ImageResources& imageResources, const VkDescriptorType type, const uint8_t pipelineIndex);
            static void addDependencies(std::vector<VkSubpassDependency>& dependencies, const std::vector<ResourceInfo>& resInfos);

            void createDescriptorPool();
            VkDescriptorSetLayout createDescriptorSetLayout(const VulkanPipeline::VkUniforms& uniforms, const VulkanPipeline::VkSubpassInputs& subpassInputs);
            VulkanPipelineInfo createVulkanPipelineInfo(const VulkanPipeline& pipeline, const uint8_t index);
            void writeDescriptorSet(const VkDescriptorSet descriptorSet, const VulkanPipeline::VkUniforms& uniforms, const VulkanPipeline::VkSubpassInputs& subpassInputs);

            VulkanRenderPass(const std::vector<VulkanPipeline>& pipelines, VkDevice device, VkOffset2D renderOffset, VkExtent2D renderSize);
            
            void init();

            VkDevice _device;
            VkRenderPass _renderPass;
            Framebuffer _framebuffer;
            RenderArea _renderArea;
            VkDescriptorPool _descriptorPool;

            DescriptorCounts _descriptorCounts;
            
            const std::vector<VulkanPipeline>& _pipelines;
            std::vector<VulkanPipelineInfo> _vkPipelines;
    };
};

#endif