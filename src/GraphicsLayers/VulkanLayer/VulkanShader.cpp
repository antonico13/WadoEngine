#include "VulkanShader.h"

namespace Wado::GAL::Vulkan {

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

    // Vulkan Shader functions now 

    void VulkanShader::addUniformDescription(spirv_cross::Compiler& spirvCompiler, const spirv_cross::SmallVector<spirv_cross::Resource>& resources, const VkDescriptorType descType, const VkShaderStageFlagBits stageFlag) {
        for (const spirv_cross::Resource &resource : resources) {

            if ( ((descType == VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER) || (descType == VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER)) &&
                (spirvCompiler.get_type(resource.type_id).image.dim != spv::Dim::DimBuffer) ) {
                // We only care about buffer dimension image resources when looking at texel buffers
                continue;
            };

            uint32_t decorationSet = spirvCompiler.get_decoration(resource.id, spv::DecorationDescriptorSet);
            uint32_t decorationBinding = spirvCompiler.get_decoration(resource.id, spv::DecorationBinding);

            // This will create the map if it does not exist 
            VkDescriptorSetLayoutMap& setLayout = _shaderLayoutMap[decorationSet];

            // Make a new binding 
            VkDescriptorSetLayoutBinding& binding = setLayout[decorationBinding];

            binding.descriptorType = descType;
            binding.binding = decorationBinding; 
            binding.descriptorCount = spirvCompiler.get_type(resource.type_id).array[0]; // TODO:: Should I deal with arrays of arrays here?
            binding.stageFlags = stageFlag;
            binding.pImmutableSamplers = nullptr; // TODO, when is this not?
            // This can be used when combining maps 
            // _descriptorSetBindings[decorationSet - 1][decorationBinding].stageFlags |= stageFlag;
            // _uniforms[decorationSet - 1][decorationBinding].binding.stageFlags |= stageFlag; 
        };
    };


    void VulkanVertexShader::generateVertexParams() {
        // Create compiler object for the SPIR-V bytecode
        spirv_cross::Compiler spirvCompiler(_bytecode.spirvWords, _bytecode.wordCount);
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


    void VulkanFragmentShader::generateFragmentParams() {
        // Create compiler object for the SPIR-V bytecode
        spirv_cross::Compiler spirvCompiler(_bytecode.spirvWords, _bytecode.wordCount);
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
            uint32_t decorationSet = spirvCompiler.get_decoration(resource.id, spv::DecorationDescriptorSet);
            uint32_t decorationBinding = spirvCompiler.get_decoration(resource.id, spv::DecorationBinding);
            uint32_t decorationIndex = spirvCompiler.get_decoration(resource.id, spv::DecorationInputAttachmentIndex);


            // This will create the map if it does not exist 
            VkDescriptorSetLayoutMap& setLayout = _shaderLayoutMap[decorationSet];

            // Make a new binding 
            VkDescriptorSetLayoutBinding& binding = setLayout[decorationBinding];

            binding.descriptorType = VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT;
            binding.binding = decorationBinding; 
            binding.descriptorCount = 1; // This is always one. 
            binding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT; // Can only appear in fragment 
            binding.pImmutableSamplers = nullptr; // TODO, when is this not?

            VkSubpassInput subpassInput{};

            subpassInput.binding = binding;
            subpassInput.decorationIndex = decorationIndex;
            subpassInput.decorationSet = decorationSet;

            // By construction the names have to be unique 
            _subpassInputs[decorationSet] = subpassInput; 
        };

        // Process fragment outputs now 
        for (const spirv_cross::Resource &resource : resources.stage_outputs) {
            VkFragmentOutput fragOutput{};
            fragOutput.decorationLocation = spirvCompiler.get_decoration(resource.id, spv::DecorationLocation); // Only care about location for now 
            // By construction the names have to be unique 
            _fragmentOutputs[fragOutput.decorationLocation] = fragOutput; 
        };
    };

    VulkanShader::VulkanShader(const VulkanLayer::SPIRVShaderBytecode& bytecode, VkShaderModule shaderModule) : _bytecode(bytecode), _shaderModule(shaderModule) { };


    VulkanVertexShader::VulkanVertexShader(const VulkanLayer::SPIRVShaderBytecode& bytecode, VkShaderModule shaderModule) : VulkanShader(bytecode, shaderModule) {
        generateVertexParams();
    };

    VulkanVertexShader::~VulkanVertexShader() {

    };


    VulkanFragmentShader::VulkanFragmentShader(const VulkanLayer::SPIRVShaderBytecode& bytecode, VkShaderModule shaderModule) : VulkanShader(bytecode, shaderModule) {
        generateFragmentParams();
    };
    
    VulkanFragmentShader::~VulkanFragmentShader() {

    };
};