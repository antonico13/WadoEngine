#ifndef WADO_VULKAN_PIPELINE
#define WADO_VULKAN_PIPELINE

#include "vulkan/vulkan.h"
#include "spirv.h"
#include "spirv_cross.hpp"

#include "WdPipeline.h"
#include "Shader.h"

#include "VulkanLayer.h"

#include <string>
#include <vector>
#include <map>
#include <tuple>

namespace Wado::GAL::Vulkan {
    // Forward declaration
    class VulkanLayer;
    class VulkanRenderPass;

    class VulkanPipeline : public WdPipeline {
        public:
            friend class VulkanLayer;
            friend class VulkanRenderPass;

            void setUniform(const WdStage stage, const std::string& paramName, const WdShaderResource& resource) override;

            void setUniform(const WdStage stage, const std::string& paramName, const std::vector<WdShaderResource>& resources) override;

            void addToUniform(const WdStage stage, const std::string& paramName, const WdShaderResource& resource, int index = UNIFORM_END) override;

            void setFragmentOutput(const std::string& paramName, Memory::WdClonePtr<WdImage> image) override;

            void setDepthStencilResource(Memory::WdClonePtr<WdImage> image) override;
        
        private:

            VulkanPipeline(SPIRVShaderByteCode vertexShader, SPIRVShaderByteCode fragmentShader, VkViewport viewport, VkRect2D scissor, VkDevice device);
            
            // Uniforms can alias in Vulkan, so they are identified through a 2 step map, first 
            // They get a unique ID based on their binding and set
            using VkUniform = struct VkUniform {
                VkDescriptorSetLayoutBinding binding;
                uint32_t decorationSet;
                //uint32_t decorationLocation; Not valid in Vulkan GLSL
                std::vector<WdResourceID> resourceIDs; // Resource IDs, used for synchronisation 
                std::vector<WdShaderResource> resources; // concrete resources, used for descriptor write creation 
            };
            
            using VkUniformAddress = std::tuple<uint32_t, uint32_t>; // (Set, Binding). These are the only unique identifiers of a uniform in Vulkan GLSL
            using VkUniformIdent = std::tuple<const std::string&, VkShaderStageFlagBits>; // Stage resource is in + uniform name as to not alias 
            
            using VkUniformAddresses = std::map<VkUniformIdent, VkUniformAddress>;
            using VkSetUniforms = std::map<uint32_t, VkUniform>;
            using VkUniforms = std::vector<VkSetUniforms>; // Index by set number, -> (map of binding -> uniform). TODO: Only support continous sets right now, starting from index 1 

            using VkSubpassInput = struct VkSubpassInput {
                VkDescriptorSetLayoutBinding binding;
                uint8_t decorationSet;
                uint8_t decorationIndex; // This index corresponds to the attach index in the pInputAttachments struct 
                Memory::WdClonePtr<WdImage> resource; // This one has to be a concrete resource since it's set at the framebuffer level, also for dependencies 
            }; 

            using VkSubpassInputs = std::map<std::string, VkSubpassInput>;

            using VkFragmentOutput = struct VkFragmentOutput { 
                uint8_t decorationLocation; // Needed in order to create refs and layouts
                Memory::WdClonePtr<WdImage> resource; 
            }; 

            using VkFragmentOutputs = std::map<std::string, VkFragmentOutput>;

            void generateVertexParams();
            void generateFragmentParams();

            void addUniformDescription(spirv_cross::Compiler& spirvCompiler, const spirv_cross::SmallVector<spirv_cross::Resource>& resources, const VkDescriptorType descType, const VkShaderStageFlagBits stageFlag);

            void createDescriptorSetLayouts();

            void createPipelineLayout();

            VkUniformAddresses _uniformAddresses;
            VkUniforms _uniforms;

            std::vector<VkVertexInputAttributeDescription> _vertexInputAttributes;
            VkVertexInputBindingDescription _vertexInputBindingDesc;
        
            std::vector<std::vector<VkDescriptorSetLayoutBinding>> _descriptorSetBindings; // bindings for all decsriptor sets
            std::vector<VkDescriptorSetLayout> _descriptorSetLayouts;
            VkPipelineLayout _pipelineLayout;

            VkSubpassInputs _subpassInputs;
            VkFragmentOutputs _fragmentOutputs;

            Memory::WdClonePtr<WdImage> _depthStencilResource; // By default is set as invalid pointer, if it's ever set before pipeline is created it will be used in Render Pass creation

            SPIRVShaderByteCode _spirvVertexShader;
            SPIRVShaderByteCode _spirvFragmentShader;
            VkViewport _viewport;
            VkRect2D _scissor;
            VkDevice _device;
    };
};

#endif