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

            VulkanPipeline(SPIRVShaderByteCode vertexShader, SPIRVShaderByteCode fragmentShader, VkViewport viewport, VkRect2D scissor);
            
            // Uniforms can alias in Vulkan, so they are identified through a 2 step map, first 
            // They get a unique ID based on their binding, set and location.
            using VkUniform = struct VkUniform {
                VkDescriptorType descType;
                uint32_t decorationSet;
                uint32_t decorationBinding;
                uint32_t decorationLocation;
                uint32_t resourceCount;
                VkShaderStageFlags stages;
                std::vector<WdShaderResource> resources;
            };
            
            using VkUniformAddress = std::tuple<uint32_t, uint32_t, uint32_t>; // (Set, Binding, Location)
            using VkUniformIdent = const std::string&; // Stage resource is in + uniform name as to not alias 
            
            using VkUniformAddresses = std::map<VkUniformIdent, VkUniformAddress>;
            using VkUniforms = std::map<VkUniformAddress, VkUniform>;

            using VkVertexInput = struct VkVertexInput {
                uint8_t decorationLocation;
                uint8_t decorationBinding; // TODO: look into different vertex bindings 
                uint8_t offset;
                size_t size;
                VkFormat format;
            };

            using VkVertexInputs = struct {
                std::vector<VkVertexInput> inputs;
                size_t totalSize;
            };

            using VkSubpassInput = struct VkSubpassInput { // Vulkan only 
                uint8_t decorationSet;
                uint8_t decorationBinding;
                uint8_t decorationLocation;
                uint8_t decorationIndex; // This index corresponds to the attach index in the pInputAttachments struct 
                Memory::WdClonePtr<WdImage> resource;
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
            void createVertexAttributeDescriptionsAndBinding();

            VkUniformAddresses _uniformAddresses;
            VkUniforms _uniforms;
            VkVertexInputs _vertexInputs;
            std::vector<VkVertexInputAttributeDescription> _vertexInputAttributes;
            VkVertexInputBindingDescription _vertexInputBindingDesc;

            VkSubpassInputs _subpassInputs; // These are fragment only
            VkFragmentOutputs _fragmentOutputs;

            Memory::WdClonePtr<WdImage> _depthStencilResource; // By default is set as invalid pointer, if it's ever set before pipeline is created it will be used in Render Pass creation

            SPIRVShaderByteCode _spirvVertexShader;
            SPIRVShaderByteCode _spirvFragmentShader;
            VkViewport _viewport;
            VkRect2D _scissor;
    };
};

#endif