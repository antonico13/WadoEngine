#include "VulkanPipeline.h"

namespace Wado::GAL::Vulkan {

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

    VulkanPipeline::VulkanPipeline(SPIRVShaderByteCode vertexShader, SPIRVShaderByteCode fragmentShader, VkViewport viewport, VkRect2D scissor) : 
        _spirvVertexShader(vertexShader), _spirvFragmentShader(fragmentShader),
        _viewport(viewport), _scissor(scissor) {
        
        generateVertexParams();
        generateFragmentParams();
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
            uint32_t decorationLocation = spirvCompiler.get_decoration(resource.id, spv::DecorationLocation); 

            VkUniformAddress address(decorationSet, decorationBinding, decorationLocation); 

            // TODO: maybe there is a better way than map look up 
            if (_uniforms.find(address) == _uniforms.end()) {
                // First time seeing this uniform, need to add its entry 
                VkUniform uniform{}; 
                uniform.descType = descType; 
                uniform.decorationSet = decorationSet;
                uniform.decorationBinding = decorationBinding;
                uniform.decorationLocation = decorationLocation;
                uniform.resourceCount = spirvCompiler.get_type(resource.type_id).array[0]; 
                uniform.resources.resize(uniform.resourceCount);
                uniform.stages = 0;

                _uniforms[address] = uniform; // TODO: this could be move actually
            }

            _uniforms[address].stages |= stageFlag;
            
            // Build unique uniform ID
            VkUniformIdent stageIdent = std::to_string(stageFlag) + resource.name;
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

        // process vertex input now 
        for (const spirv_cross::Resource &resource : resources.stage_inputs) {
            VkVertexInput vertexInput;
            vertexInput.decorationBinding = spirvCompiler.get_decoration(resource.id, spv::DecorationBinding); // TODO: look into how this is used 
            vertexInput.decorationLocation = spirvCompiler.get_decoration(resource.id, spv::DecorationLocation); 
            vertexInput.offset = spirvCompiler.get_decoration(resource.id, spv::DecorationOffset); 

            const spirv_cross::SPIRType& type = spirvCompiler.get_type(resource.type_id);
            uint32_t vecSize = type.vecsize;
            spirv_cross::SPIRType::BaseType baseType = type.basetype;

            vertexInput.size = vecSize * SPIRVBaseToSize[baseType];
            vertexInput.format = SPIRVtoVkFormat[vecSize - 1][baseType];

            _vertexInputs.totalSize += vertexInput.size;
            _vertexInputs.inputs.push_back(vertexInput);
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
        addUniformDescription(spirvCompiler, resources.storage_buffers, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER , VK_SHADER_STAGE_FRAGMENT_BIT); // TODO: these can also be dynamic 

        // Process subpass inputs now, which are fragment shader only 
        for (const spirv_cross::Resource &resource : resources.subpass_inputs) {
            VkSubpassInput subpassInput{};
            subpassInput.decorationBinding = spirvCompiler.get_decoration(resource.id, spv::DecorationBinding); 
            subpassInput.decorationLocation = spirvCompiler.get_decoration(resource.id, spv::DecorationLocation);
            subpassInput.decorationIndex = spirvCompiler.get_decoration(resource.id, spv::DecorationInputAttachmentIndex);
            subpassInput.decorationSet = spirvCompiler.get_decoration(resource.id, spv::DecorationDescriptorSet);
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
};