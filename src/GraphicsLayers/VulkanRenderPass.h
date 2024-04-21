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
                std::vector<VkDescriptorSetLayout> descriptorSetLayouts;
                std::vector<VkDescriptorSet> descriptorSets;
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

            using VertexInputDesc = std::tuple<std::vector<VkVertexInputAttributeDescription>, VkVertexInputBindingDescription>;

            using Framebuffer = struct Framebuffer {
                std::vector<VkImageView> attachmentViews;
                std::vector<VkClearValue> clearValues;
                VkFramebuffer framebuffer;
            };

            using RenderArea = struct RenderArea {
                VkExtent2D offset;
                VkExtent2D extent;
            };

            // Wd to Vk Translation utils

            /*inline static const WdPipeline::WdShaderParameterTypeMask _imgReadMask = WdPipeline::WdShaderParameterType::WD_SAMPLED_IMAGE & WdPipeline::WdShaderParameterType::WD_TEXTURE_IMAGE & WdPipeline::WdShaderParameterType::WD_STORAGE_IMAGE & WdPipeline::WdShaderParameterType::WD_SUBPASS_INPUT;
            inline static const WdPipeline::WdShaderParameterTypeMask _imgWriteMask = WdPipeline::WdShaderParameterType::WD_STORAGE_IMAGE & WdPipeline::WdShaderParameterType::WD_STAGE_OUTPUT;

            inline static const WdPipeline::WdShaderParameterTypeMask _bufReadMask = WdPipeline::WdShaderParameterType::WD_SAMPLED_BUFFER & WdPipeline::WdShaderParameterType::WD_BUFFER_IMAGE & WdPipeline::WdShaderParameterType::WD_UNIFORM_BUFFER & WdPipeline::WdShaderParameterType::WD_STORAGE_BUFFER;
            inline static const WdPipeline::WdShaderParameterTypeMask _bufWriteMask = WdPipeline::WdShaderParameterType::WD_BUFFER_IMAGE & WdPipeline::WdShaderParameterType::WD_STORAGE_BUFFER;

            inline static const WdPipeline::WdShaderParameterTypeMask _bufferMask = WdPipeline::WdShaderParameterType::WD_SAMPLED_BUFFER & WdPipeline::WdShaderParameterType::WD_BUFFER_IMAGE & WdPipeline::WdShaderParameterType::WD_UNIFORM_BUFFER & WdPipeline::WdShaderParameterType::WD_STORAGE_BUFFER;
            inline static const WdPipeline::WdShaderParameterTypeMask _imageMask = WdPipeline::WdShaderParameterType::WD_SAMPLED_IMAGE & WdPipeline::WdShaderParameterType::WD_TEXTURE_IMAGE & WdPipeline::WdShaderParameterType::WD_STORAGE_IMAGE & WdPipeline::WdShaderParameterType::WD_SUBPASS_INPUT & WdPipeline::WdShaderParameterType::WD_STAGE_OUTPUT; */

            /*static const VkAccessFlags WdShaderParamTypeToAccess[]; 

            static const VkDescriptorType WdShaderParamTypeToVkDescriptorType[];

            static VkShaderStageFlagBits WdStagesToVkStages(const WdStageMask stages);

            static VkPipelineStageFlags ResInfoToVkStage(const ResourceInfo& resInfo); */

            //static void addDependencies(std::vector<VkSubpassDependency>& dependencies, std::vector<ResourceInfo>& resInfos, uint32_t readMask, uint32_t writeMask);

            static void updateFragmentOutputAttachements(const VulkanPipeline::VkFragmentOutputs& resourceMap, AttachmentMap& attachments, uint8_t& attachmentIndex, std::vector<VkAttachmentReference>& attachmentRefs, std::vector<VkImageView>& framebuffer, VkImageLayout layout);
            static void updateSubpassInputAttachements(const VulkanPipeline::VkSubpassInputs& resourceMap, AttachmentMap& attachments, uint8_t& attachmentIndex, std::vector<VkAttachmentReference>& attachmentRefs, std::vector<VkImageView>& framebuffer, VkImageLayout layout);

            /*static void updateUniformResources(const WdPipeline::WdUniforms& resourceMap, ImageResources& imageResources, BufferResources& bufferResources, uint8_t pipelineIndex);
            
            template <class T>
            static void updateAttachmentResources(const T& resourceMap, ImageResources& imageResources, uint8_t pipelineIndex);
            
            VkDescriptorSetLayout createDescriptorSetLayout(const WdPipeline::WdUniforms& uniforms, const WdPipeline::WdSubpassInputs& subpassInputs);
            
            static VertexInputDesc createVertexAttributeDescriptionsAndBinding(const WdPipeline::WdVertexInputs& vertexInputs);

            static void addDescriptorPoolSizes(const WdPipeline& pipeline, std::vector<VkDescriptorPoolSize>& poolSizes);

            void writeDescriptorSets(const std::vector<VkDescriptorSet>& descriptorSets, const WdPipeline::WdUniforms& uniforms, const WdPipeline::WdSubpassInputs& subpassInputs) const;

            void createDescriptorPool();

            VulkanPipelineInfo createVulkanPipeline(const VulkanPipeline& pipeline, uint8_t index); */

            VulkanRenderPass(const std::vector<VulkanPipeline>& pipelines, VkDevice device);
            
            void init();

            VkDevice _device;
            VkRenderPass _renderPass;
            Framebuffer _framebuffer; // TODO: need to actually create the framebuffer object
            RenderArea _renderArea;
            VkDescriptorPool _descriptorPool;
            
            const std::vector<VulkanPipeline>& _pipelines;
            std::vector<VulkanPipelineInfo> _vkPipelines;
    };
};

#endif