#ifndef WADO_VULKAN_SHADER_H
#define WADO_VULKAN_SHADER_H

#include "WdShader.h"
#include "VulkanLayer.h"

#include "vulkan/vulkan.h"
#include "spirv.h"
#include "spirv_cross.hpp"

#include <unordered_map>
#include <vector>

namespace Wado::GAL::Vulkan {

    class VulkanShader : public WdShader {
        public:
            friend class VulkanLayer;

            // These are immediate-mode 
            // TODO: make sure to auto-infer layout 
            void setShaderResource(const WdShaderResourceLocation location, const WdImageResourceHandle image) override;
            // For combined image-samplers 
            void setShaderResource(const WdShaderResourceLocation location, const WdImageResourceHandle image, const WdSamplerHandle sampler) override;
            // For pure sampler resources 
            void setShaderResource(const WdShaderResourceLocation location, const WdSamplerHandle sampler) override;

            void setShaderResource(const WdShaderResourceLocation location, const WdBufferResourceHandle bufferResource) override;

            void setShaderResource(const WdShaderResourceLocation location, const WdBuffer buffer, const WdSize offset = 0, const WdSize range = 0) override;
        protected:
            void addUniformDescription(spirv_cross::Compiler& spirvCompiler, const spirv_cross::SmallVector<spirv_cross::Resource>& resources, const VkDescriptorType descType, const VkShaderStageFlagBits stageFlag);

            void createDescriptorSetLayouts();

            const VkShaderModule _shaderModule;

            const VulkanLayer::SPIRVShaderBytecode _bytecode;

            // TODO: uint64_t size ok?
            using VkDescriptorSetLayoutMap = std::unordered_map<uint64_t, VkDescriptorSetLayoutBinding>;
            using VkShaderLayoutMap = std::unordered_map<uint64_t, VkDescriptorSetLayoutMap>;

            VkShaderLayoutMap _shaderLayoutMap;
    };

    class VulkanVertexShader : public WdVertexShader, public VulkanShader {
        public:
            friend class VulkanLayer;

        private:
            VulkanVertexShader(const std::vector<char>& byteCode, VkShaderModule shaderModule);
            ~VulkanVertexShader();

            void generateVertexParams();

            std::vector<VkVertexInputAttributeDescription> _vertexInputAttributes;
            VkVertexInputBindingDescription _vertexInputBindingDesc;
    };

    class VulkanFragmentShader : public WdFragmentShader, public VulkanShader {
        public:
            friend class VulkanLayer;
        private:
            VulkanFragmentShader(const std::vector<char>& byteCode, VkShaderModule shaderModule);
            ~VulkanFragmentShader();

            void generateFragmentParams();

            using VkSubpassInput = struct VkSubpassInput {
                VkDescriptorSetLayoutBinding binding;
                uint8_t decorationSet;
                uint8_t decorationIndex; // This index corresponds to the attach index in the pInputAttachments struct 
                //Memory::WdClonePtr<WdImage> resource; // This one has to be a concrete resource since it's set at the framebuffer level, also for dependencies 
            }; 

            using VkSubpassInputs = std::unordered_map<uint8_t, VkSubpassInput>;

            using VkFragmentOutput = struct VkFragmentOutput { 
                uint8_t decorationLocation; // Needed in order to create refs and layouts, analog to "Location" in the actual shader
                //Memory::WdClonePtr<WdImage> resource; 
            }; 

            using VkFragmentOutputs = std::unordered_map<uint8_t, VkFragmentOutput>;

            VkSubpassInputs _subpassInputs;
            VkFragmentOutputs _fragmentOutputs;
    };

};
#endif