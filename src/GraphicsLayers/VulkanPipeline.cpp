#include "VulkanPipeline.h"

#include <assert.h> 
#include <stdexcept>

namespace Wado::GAL::Vulkan {

    void VulkanPipeline::setUniform(const WdStage stage, const std::string& paramName, const WdShaderResource& resource) {
        // If fragment, look first in subpassInputs before going to the general case 
        if (stage == WdStage::Fragment) {

            VkSubpassInputs::iterator it = _subpassInputs.find(paramName);
            
            if (it != _subpassInputs.end()) { // Is a subpass input, need to set its resource 
                // TODO: Throw error if image not provided 
                it->second.resource = resource.imageResource.image;
                return;
            };
        };

        // If above fails, go to the general uniform case 
        setUniform(stage, paramName, {resource}); 
    };

    static const VkShaderStageFlagBits WdShaderStageToVkShaderStage[] = {
        VK_SHADER_STAGE_ALL, // Corresponds to WdStage::Unknown
        VK_SHADER_STAGE_VERTEX_BIT, // Corresponds to WdStage::Vertex
        VK_SHADER_STAGE_FRAGMENT_BIT, // Corresponds to WdStage::Fragment
    };

    void VulkanPipeline::setUniform(const WdStage stage, const std::string& paramName, const std::vector<WdShaderResource>& resources) {
        VkUniformIdent uniformIdent(paramName, WdShaderStageToVkShaderStage[stage]);
        VkUniformAddresses::iterator it = _uniformAddresses.find(uniformIdent);

        if (it != _uniformAddresses.end()) { // Found, can set resource 
            uint32_t uniformSet = std::get<0>(it->second);
            uint32_t uniformBinding = std::get<1>(it->second);
            // TODO: bounds check here 
            VkUniform& uniform = _uniforms[uniformSet][uniformBinding];
            
            if (uniform.binding.descriptorCount < resources.size()) {
                throw std::runtime_error("Cannot set uniform " + paramName + " values since provided resource array has greater size than the maximum resoucre count for this uniform.");
            };
            // TODO: type check here 
            // TODO: set resource IDs too?
            uniform.resources = resources;
        };

        throw std::runtime_error("Uniform " + paramName + "not found.");
    };
    
    // Only for array uniforms 
    void VulkanPipeline::addToUniform(const WdStage stage, const std::string& paramName, const WdShaderResource& resource, int index) {
        VkUniformIdent uniformIdent(paramName, WdShaderStageToVkShaderStage[stage]);
        VkUniformAddresses::iterator it = _uniformAddresses.find(uniformIdent);
        
        if (it != _uniformAddresses.end()) { // found, can add to resource 
            uint32_t uniformSet = std::get<0>(it->second);
            uint32_t uniformBinding = std::get<1>(it->second);
            // TODO: bounds check here 
            VkUniform& uniform = _uniforms[uniformSet][uniformBinding];
            // Checks here
            if (uniform.binding.descriptorCount == 1) {
                throw std::runtime_error("Cannot add to uniform " + paramName + ", since it is of non-array type.");
            };

            if (uniform.binding.descriptorCount < index) {
                throw std::runtime_error("Cannot add to uniform " + paramName + ", since provided index is out of bounds.");
                return;
            };

            if (index == UNIFORM_END) {
                // Need to push back in this case 

                if (uniform.binding.descriptorCount == uniform.resources.size()) {
                    throw std::runtime_error("Cannot add to uniform " + paramName + ", all of its values have already been set.");
                };

                uniform.resources.push_back(resource);
            } else {
                // TODO: this is terrible but it might work, fix pls 
                std::replace(uniform.resources.begin() + index, uniform.resources.begin() + index + 1, uniform.resources[index], resource); 
            };
        };
        
        throw std::runtime_error("Uniform " + paramName + "not found.");
    };

    void VulkanPipeline::setFragmentOutput(const std::string& paramName, Memory::WdClonePtr<WdImage> image) {
        VkFragmentOutputs::iterator it = _fragmentOutputs.find(paramName);
        
        if (it != _fragmentOutputs.end()) {
            it->second.resource = image;
        };

        throw std::runtime_error("There is no fragment output argument with the name " + paramName);
    };

    void VulkanPipeline::setDepthStencilResource(Memory::WdClonePtr<WdImage> image) {
        _depthStencilResource = image;
    };
        
    // Internal functions and helpers

    // index by vec size first, then by base type 
    static const VkFormat SPIRVtoVkFormat[4][15] = {
        {VK_FORMAT_UNDEFINED, VK_FORMAT_UNDEFINED, VK_FORMAT_R8_UNORM, VK_FORMAT_R8_SINT, VK_FORMAT_R8_UINT, VK_FORMAT_R16_SINT, VK_FORMAT_R16_UINT, VK_FORMAT_R32_SINT, VK_FORMAT_R32_UINT, VK_FORMAT_R64_SINT, VK_FORMAT_R64_UINT, VK_FORMAT_R32_UINT, VK_FORMAT_R16_SFLOAT, VK_FORMAT_R32_SFLOAT, VK_FORMAT_R64_SFLOAT,}, // size 1
        {VK_FORMAT_UNDEFINED, VK_FORMAT_UNDEFINED, VK_FORMAT_R8G8_UNORM, VK_FORMAT_R8G8_SINT, VK_FORMAT_R8G8_UINT, VK_FORMAT_R16G16_SINT, VK_FORMAT_R16G16_UINT, VK_FORMAT_R32G32_SINT, VK_FORMAT_R32G32_UINT, VK_FORMAT_R64G64_SINT, VK_FORMAT_R64G64_UINT, VK_FORMAT_R32G32_UINT, VK_FORMAT_R16G16_SFLOAT, VK_FORMAT_R32G32_SFLOAT, VK_FORMAT_R64G64_SFLOAT,}, // size 2
        {VK_FORMAT_UNDEFINED, VK_FORMAT_UNDEFINED, VK_FORMAT_R8G8B8_UNORM, VK_FORMAT_R8G8B8_SINT, VK_FORMAT_R8G8B8_UINT, VK_FORMAT_R16G16B16_SINT, VK_FORMAT_R16G16B16_UINT, VK_FORMAT_R32G32B32_SINT, VK_FORMAT_R32G32B32_UINT, VK_FORMAT_R64G64B64_SINT, VK_FORMAT_R64G64B64_UINT, VK_FORMAT_R32G32B32_UINT, VK_FORMAT_R16G16B16_SFLOAT, VK_FORMAT_R32G32B32_SFLOAT, VK_FORMAT_R64G64B64_SFLOAT,}, // size 3
        {VK_FORMAT_UNDEFINED, VK_FORMAT_UNDEFINED, VK_FORMAT_R8G8B8A8_UNORM, VK_FORMAT_R8G8B8A8_SINT, VK_FORMAT_R8G8B8A8_UINT, VK_FORMAT_R16G16B16A16_SINT, VK_FORMAT_R16G16B16A16_UINT, VK_FORMAT_R32G32B32A32_SINT, VK_FORMAT_R32G32B32A32_UINT, VK_FORMAT_R64G64B64A64_SINT, VK_FORMAT_R64G64B64A64_UINT, VK_FORMAT_R32G32B32A32_UINT, VK_FORMAT_R16G16B16A16_SFLOAT, VK_FORMAT_R32G32B32A32_SFLOAT, VK_FORMAT_R64G64B64A64_SFLOAT,}, // size 4
    };

    // size in bytes 
    static const size_t SPIRVBaseToSize[] = {
        0, // Unknown
		0, // Void
		4, // Boolean
		4, // SByte,
		4, // UByte,
		4, // Short,
		4, // UShort,
		4, // Int,
		4, // UInt,
		8, // Int64,
		8, // UInt64,
		4, // AtomicCounter,
		4, // Half,
		4, // Float,
		8, // Double,
    };

    VulkanPipeline::VulkanPipeline(SPIRVShaderByteCode vertexShader, SPIRVShaderByteCode fragmentShader, VkViewport viewport, VkRect2D scissor, VkDevice device) : 
        _spirvVertexShader(vertexShader), _spirvFragmentShader(fragmentShader),
        _viewport(viewport), _scissor(scissor), _device(device) {
        
        generateVertexParams();
        generateFragmentParams();
        createDescriptorSetLayouts();
        createPipelineLayout();
    };

    void VulkanPipeline::addUniformDescription(spirv_cross::Compiler& spirvCompiler, const spirv_cross::SmallVector<spirv_cross::Resource>& resources, const VkDescriptorType descType, const VkShaderStageFlagBits stageFlag) {
        for (const spirv_cross::Resource &resource : resources) {

            if ( ((descType == VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER) || (descType == VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER)) &&
                (spirvCompiler.get_type(resource.type_id).image.dim != spv::Dim::DimBuffer) ) {
                // We only care about buffer dimension image resources when looking at texel buffers
                continue;
            };

            uint32_t decorationSet = spirvCompiler.get_decoration(resource.id, spv::DecorationDescriptorSet);
            uint32_t decorationBinding = spirvCompiler.get_decoration(resource.id, spv::DecorationBinding);

            VkUniformAddress address(decorationSet, decorationBinding);

            if (_uniforms.size() < decorationSet) {
                // This set has never been seen before, need to add it to the general vector,
                // and the layout vector
                _uniforms.resize(decorationSet); // TODO: Idk if this is the best resize
                _descriptorSetBindings.resize(decorationSet);
                _uniforms[decorationSet - 1] = VkSetUniforms(); // Sets start from 1, index from 0 here 
            };

            // TODO: Not sure if I should combine maps or not 
            // TODO: bindings right now don't have to be continous, and this works. Should they be continous?
            if (_uniforms[decorationSet - 1].find(decorationBinding) == _uniforms[decorationSet].end()) {
                // First time seeing this uniform, need to add its entry 
                VkUniform uniform{}; 
                uniform.binding.descriptorType = descType;
                uniform.binding.binding = decorationBinding; 
                uniform.binding.descriptorCount = spirvCompiler.get_type(resource.type_id).array[0]; // TODO:: Should I deal with arrays of arrays here?
                uniform.binding.stageFlags = stageFlag;
                uniform.binding.pImmutableSamplers = nullptr; // TODO, when is this not?

                uniform.decorationSet = decorationSet;
                uniform.resourceIDs.resize(uniform.binding.descriptorCount);
                uniform.resources.resize(uniform.binding.descriptorCount);
                
                if (_descriptorSetBindings[decorationSet - 1].size() <= decorationBinding) {
                    _descriptorSetBindings[decorationSet - 1].resize(decorationBinding + 1); // TODO: lots of resizes, bad
                };
                _descriptorSetBindings[decorationSet - 1][decorationBinding] = uniform.binding;
                _uniforms[decorationSet - 1][decorationBinding] = uniform; // TODO: this could be move actually
            } else {
                _descriptorSetBindings[decorationSet - 1][decorationBinding].stageFlags |= stageFlag;
                _uniforms[decorationSet - 1][decorationBinding].binding.stageFlags |= stageFlag; 
            };
            
            // Build unique uniform ID
            VkUniformIdent stageIdent(resource.name, stageFlag);
            _uniformAddresses[stageIdent] = address; 
        };
    };

    void VulkanPipeline::generateVertexParams() {
        // Create compiler object for the SPIR-V bytecode
        spirv_cross::Compiler spirvCompiler(_spirvVertexShader.spirvWords, _spirvVertexShader.wordCount);
        spirv_cross::ShaderResources resources = spirvCompiler.get_shader_resources();

        // Check all non-subpass input uniforms 
        addUniformDescription(spirvCompiler, resources.sampled_images, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_VERTEX_BIT);
        addUniformDescription(spirvCompiler, resources.separate_images, VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, VK_SHADER_STAGE_VERTEX_BIT);
        addUniformDescription(spirvCompiler, resources.storage_images, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, VK_SHADER_STAGE_VERTEX_BIT);
        addUniformDescription(spirvCompiler, resources.separate_images, VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, VK_SHADER_STAGE_VERTEX_BIT);
        addUniformDescription(spirvCompiler, resources.storage_images, VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, VK_SHADER_STAGE_VERTEX_BIT);
        addUniformDescription(spirvCompiler, resources.separate_samplers, VK_DESCRIPTOR_TYPE_SAMPLER, VK_SHADER_STAGE_VERTEX_BIT);
        addUniformDescription(spirvCompiler, resources.uniform_buffers, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT); // TODO: look into dynamic uniform buffer here 
        //addUniformDescription(push_constant_buffers, WD_PUSH_CONSTANT); TODO: These need to be handled separately 
        addUniformDescription(spirvCompiler, resources.storage_buffers, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER , VK_SHADER_STAGE_VERTEX_BIT); // TODO: these can also be dynamic 

        // process vertex inputs now and build input layout 
        _vertexInputBindingDesc.binding = 0; // TODO: how to handle this? In what case do we have different bindings?
        _vertexInputBindingDesc.inputRate = VK_VERTEX_INPUT_RATE_VERTEX; // TODO: when is this instance?
        _vertexInputBindingDesc.stride = 0;

        for (const spirv_cross::Resource &resource : resources.stage_inputs) {
            
            VkVertexInputAttributeDescription attributeDescription;
            attributeDescription.binding = spirvCompiler.get_decoration(resource.id, spv::DecorationBinding); // TODO: look into how this is actually used 
            attributeDescription.location = spirvCompiler.get_decoration(resource.id, spv::DecorationLocation);
            attributeDescription.offset = spirvCompiler.get_decoration(resource.id, spv::DecorationOffset); // TODO: i think this is talking about the layout offset qualifier, not the vertex input offset, so it's wrong

            const spirv_cross::SPIRType& type = spirvCompiler.get_type(resource.type_id);
            uint32_t vecSize = type.vecsize;
            spirv_cross::SPIRType::BaseType baseType = type.basetype;
            
            attributeDescription.format = SPIRVtoVkFormat[vecSize - 1][baseType];

            _vertexInputAttributes.push_back(attributeDescription);

            _vertexInputBindingDesc.stride += vecSize * SPIRVBaseToSize[baseType];
        };
    };

    void VulkanPipeline::generateFragmentParams() {
        // Create compiler object for the SPIR-V bytecode
        spirv_cross::Compiler spirvCompiler( _spirvFragmentShader.spirvWords, _spirvFragmentShader.wordCount);
        spirv_cross::ShaderResources resources = spirvCompiler.get_shader_resources();

        // Check all non-subpass input uniforms 
        addUniformDescription(spirvCompiler, resources.sampled_images, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT);
        addUniformDescription(spirvCompiler, resources.separate_images, VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, VK_SHADER_STAGE_FRAGMENT_BIT);
        addUniformDescription(spirvCompiler, resources.storage_images, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, VK_SHADER_STAGE_FRAGMENT_BIT);
        addUniformDescription(spirvCompiler, resources.separate_images, VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, VK_SHADER_STAGE_FRAGMENT_BIT);
        addUniformDescription(spirvCompiler, resources.storage_images, VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, VK_SHADER_STAGE_FRAGMENT_BIT);
        addUniformDescription(spirvCompiler, resources.separate_samplers, VK_DESCRIPTOR_TYPE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT);
        addUniformDescription(spirvCompiler, resources.uniform_buffers, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_FRAGMENT_BIT); // TODO: look into dynamic uniform buffer here 
        //addUniformDescription(push_constant_buffers, WD_PUSH_CONSTANT); TODO: These need to be handled separately 
        addUniformDescription(spirvCompiler, resources.storage_buffers, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_FRAGMENT_BIT); // TODO: these can also be dynamic 

        // Process subpass inputs now, which are fragment shader only 
        for (const spirv_cross::Resource &resource : resources.subpass_inputs) {
            VkSubpassInput subpassInput{};
            subpassInput.binding.binding = spirvCompiler.get_decoration(resource.id, spv::DecorationBinding); 
            subpassInput.binding.descriptorCount = 1; // Fixed size;
            subpassInput.binding.descriptorType = VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT;
            subpassInput.binding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
            subpassInput.binding.pImmutableSamplers = nullptr;

            subpassInput.decorationIndex = spirvCompiler.get_decoration(resource.id, spv::DecorationInputAttachmentIndex);
            subpassInput.decorationSet = spirvCompiler.get_decoration(resource.id, spv::DecorationDescriptorSet);
            
            if (_descriptorSetBindings.size() < subpassInput.decorationSet) {
                _descriptorSetBindings.resize(subpassInput.decorationSet);
            };

            if (_descriptorSetBindings[subpassInput.decorationSet - 1].size() <= subpassInput.binding.binding) {
                _descriptorSetBindings[subpassInput.decorationSet - 1].resize(subpassInput.binding.binding + 1);
            };

            _descriptorSetBindings[subpassInput.decorationSet - 1][subpassInput.binding.binding] = subpassInput.binding;
            // By construction the names have to be unique 
            _subpassInputs[resource.name] = subpassInput; 
        };

        // Process fragment outputs now 
        for (const spirv_cross::Resource &resource : resources.stage_outputs) {
            VkFragmentOutput fragOutput{};
            fragOutput.decorationLocation = spirvCompiler.get_decoration(resource.id, spv::DecorationLocation); // Only care about location for now 
            // By construction the names have to be unique 
            _fragmentOutputs[resource.name] = fragOutput; 
        };
    };

    void VulkanPipeline::createDescriptorSetLayouts() {
        //_descSetBindings.resize(_uniforms.size()); // As many bindings as there are descriptor sets 
        _descriptorSetLayouts.resize(_uniforms.size());

        // All descriptor set bindings should have been created by now when the pipeline was loaded in 
        for (int descSetIndex = 0; descSetIndex < _uniforms.size(); descSetIndex++) {
            VkDescriptorSetLayoutCreateInfo layoutInfo{};
            layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
            layoutInfo.bindingCount = static_cast<uint32_t>(_descriptorSetBindings[descSetIndex].size());
            layoutInfo.pBindings = _descriptorSetBindings[descSetIndex].data();

            if (vkCreateDescriptorSetLayout(_device, &layoutInfo, nullptr, &_descriptorSetLayouts[descSetIndex]) != VK_SUCCESS) {
                throw std::runtime_error("Failed to create Vulkan descriptor set layout!");
            };
        };
    };

    void VulkanPipeline::createPipelineLayout() {
        
        VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
        pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        pipelineLayoutInfo.setLayoutCount = static_cast<uint32_t>(_descriptorSetLayouts.size());
        pipelineLayoutInfo.pSetLayouts = _descriptorSetLayouts.data();
        pipelineLayoutInfo.pushConstantRangeCount = 0; // TODO: work push constants into the workflow
        pipelineLayoutInfo.pPushConstantRanges = nullptr;

        if (vkCreatePipelineLayout(_device, &pipelineLayoutInfo, nullptr, &_pipelineLayout) != VK_SUCCESS) {
            throw std::runtime_error("Failed to create Vulkan pipeline layout!");
        };
    }
};