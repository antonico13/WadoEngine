#include "WdPipeline.h"

#include <algorithm>
#include <utility>

namespace Wado::GAL {
    // Shader Resource initalization 

    WdShaderResource::WdShaderResource(Memory::WdClonePtr<WdImage> img) : imageResource({img, WD_INVALID_HANDLE}) { };

    WdShaderResource::WdShaderResource(Memory::WdClonePtr<WdBuffer> buf) : bufferResource({buf, WD_INVALID_HANDLE}) { };
    
    WdShaderResource::WdShaderResource(WdSamplerHandle sampler) : imageResource({Memory::WdClonePtr<WdImage>(), sampler}) { };

    WdShaderResource::WdShaderResource(Memory::WdClonePtr<WdImage> img, WdSamplerHandle sampler) : imageResource({img, sampler}) { };
    
    WdShaderResource::WdShaderResource(Memory::WdClonePtr<WdBuffer> buf, WdRenderTarget bufTarget) : bufferResource({buf, bufTarget}) { };

    // Public util functions

    // Subpass inputs are also handled here 
    void WdPipeline::setUniform(const WdStage stage, const std::string& paramName, const WdShaderResource& resource) {
        // If fragment, look first in subpassInputs before going to the general case 
        if (stage == WdStage::Fragment) {
            WdSubpassInputs::iterator it = _subpassInputs.find(paramName);
            if (it != _subpassInputs.end()) { // is a subpass input 
                it->second.resource = resource.imageResource.image;
                return;
            }
        }
        // if above fails, go to the general uniform case 
        setUniform(stage, paramName, {resource});
    };

    // For array params
    void WdPipeline::setUniform(const WdStage stage, const std::string& paramName, const std::vector<WdShaderResource>& resources) {
        WdUniformAddresses::iterator it = _uniformAddresses.find({paramName, stage});
        if (it != _uniformAddresses.end()) { // found, can set resource 
            WdUniformAddress address = it->second;
            WdUniforms::iterator uniform = _uniforms.find(address);
            // this should never fail, TODO : add check here just in case (assert?)
            if (uniform->second.resourceCount < resources.size()) {
                // throw error here for setting outside bounds
                return;
            }
            uniform->second.resources = resources;
        }
        // need to throw error here if param doesnt exist 
    };

    void WdPipeline::addToUniform(const WdStage stage, const std::string& paramName, const WdShaderResource& resource, int index) {
        WdUniformAddresses::iterator it = _uniformAddresses.find({paramName, stage});
        if (it != _uniformAddresses.end()) { // found, can add to resource 
            WdUniformAddress address = it->second;
            WdUniforms::iterator uniform = _uniforms.find(address);
            if (uniform->second.resourceCount < index) {
                // throw error here, out of bounds 
                return;
            }
            if (index == UNIFORM_END) {
                // need to push back
                if (uniform->second.resourceCount == uniform->second.resources.size()) {
                    // throw error here, can't add at the end since all parameters have been set 
                    return; 
                }
                uniform->second.resources.push_back(resource);
            } else {
                // TODO: this is terrible but it might work, fix pls 
                std::replace(uniform->second.resources.begin() + index, uniform->second.resources.begin() + index + 1, uniform->second.resources[index], resource); 
            }
        // need to throw error here 
        }        
    };

    void WdPipeline::setFragmentOutput(const std::string& paramName, Memory::WdClonePtr<WdImage> resource) {
        WdFragmentOutputs::iterator it = _fragmentOutputs.find(paramName);
        if (it != _fragmentOutputs.end()) {
            it->second.resource = resource;
        }
        // error, param not found bla bla 
        // can also type check here (later)
    };

    void WdPipeline::setDepthStencilResource(Memory::WdClonePtr<WdImage> resource) {
        _depthStencilResource = resource;
    };

    // Private util functions 

    WdPipeline::WdPipeline(Shader::WdShaderByteCode vertexShader, Shader::WdShaderByteCode fragmentShader, const WdViewportProperties& viewportProperties) : _vertexShader(vertexShader), _fragmentShader(fragmentShader), _viewportProperties(viewportProperties) {
        generateVertexParams();
        generateFragmentParams();
    };

    // index by vec size first, then by base type 
    static const WdFormat SPIRVtoWdFormat[4][15] = {
        {WdFormat::WD_FORMAT_UNDEFINED, WdFormat::WD_FORMAT_UNDEFINED, WdFormat::WD_FORMAT_R8_UNORM, WdFormat::WD_FORMAT_R8_SINT, WdFormat::WD_FORMAT_R8_UINT, WdFormat::WD_FORMAT_R16_SINT, WdFormat::WD_FORMAT_R16_UINT, WdFormat::WD_FORMAT_R32_SINT, WdFormat::WD_FORMAT_R32_UINT, WdFormat::WD_FORMAT_R64_SINT, WdFormat::WD_FORMAT_R64_UINT, WdFormat::WD_FORMAT_R32_UINT, WdFormat::WD_FORMAT_R16_SFLOAT, WdFormat::WD_FORMAT_R32_SFLOAT, WdFormat::WD_FORMAT_R64_SFLOAT,}, // size 1
        {WdFormat::WD_FORMAT_UNDEFINED, WdFormat::WD_FORMAT_UNDEFINED, WdFormat::WD_FORMAT_R8G8_UNORM, WdFormat::WD_FORMAT_R8G8_SINT, WdFormat::WD_FORMAT_R8G8_UINT, WdFormat::WD_FORMAT_R16G16_SINT, WdFormat::WD_FORMAT_R16G16_UINT, WdFormat::WD_FORMAT_R32G32_SINT, WdFormat::WD_FORMAT_R32G32_UINT, WdFormat::WD_FORMAT_R64G64_SINT, WdFormat::WD_FORMAT_R64G64_UINT, WdFormat::WD_FORMAT_R32G32_UINT, WdFormat::WD_FORMAT_R16G16_SFLOAT, WdFormat::WD_FORMAT_R32G32_SFLOAT, WdFormat::WD_FORMAT_R64G64_SFLOAT,}, // size 2
        {WdFormat::WD_FORMAT_UNDEFINED, WdFormat::WD_FORMAT_UNDEFINED, WdFormat::WD_FORMAT_R8G8B8_UNORM, WdFormat::WD_FORMAT_R8G8B8_SINT, WdFormat::WD_FORMAT_R8G8B8_UINT, WdFormat::WD_FORMAT_R16G16B16_SINT, WdFormat::WD_FORMAT_R16G16B16_UINT, WdFormat::WD_FORMAT_R32G32B32_SINT, WdFormat::WD_FORMAT_R32G32B32_UINT, WdFormat::WD_FORMAT_R64G64B64_SINT, WdFormat::WD_FORMAT_R64G64B64_UINT, WdFormat::WD_FORMAT_R32G32B32_UINT, WdFormat::WD_FORMAT_R16G16B16_SFLOAT, WdFormat::WD_FORMAT_R32G32B32_SFLOAT, WdFormat::WD_FORMAT_R64G64B64_SFLOAT,}, // size 3
        {WdFormat::WD_FORMAT_UNDEFINED, WdFormat::WD_FORMAT_UNDEFINED, WdFormat::WD_FORMAT_R8G8B8A8_UNORM, WdFormat::WD_FORMAT_R8G8B8A8_SINT, WdFormat::WD_FORMAT_R8G8B8A8_UINT, WdFormat::WD_FORMAT_R16G16B16A16_SINT, WdFormat::WD_FORMAT_R16G16B16A16_UINT, WdFormat::WD_FORMAT_R32G32B32A32_SINT, WdFormat::WD_FORMAT_R32G32B32A32_UINT, WdFormat::WD_FORMAT_R64G64B64A64_SINT, WdFormat::WD_FORMAT_R64G64B64A64_UINT, WdFormat::WD_FORMAT_R32G32B32A32_UINT, WdFormat::WD_FORMAT_R16G16B16A16_SFLOAT, WdFormat::WD_FORMAT_R32G32B32A32_SFLOAT, WdFormat::WD_FORMAT_R64G64B64A64_SFLOAT,}, // size 4
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


    void WdPipeline::addUniformDescription(spirv_cross::Compiler& spirvCompiler, const spirv_cross::SmallVector<spirv_cross::Resource>& resources, const WdShaderParameterType paramType, const WdStage stage) {
        for (const spirv_cross::Resource &resource : resources) {
            if ( ((paramType == WdShaderParameterType::WD_SAMPLED_BUFFER) || (paramType == WdShaderParameterType::WD_BUFFER_IMAGE)) &&
                    (spirvCompiler.get_type(resource.type_id).image.dim != spv::Dim::DimBuffer) ) {
                // We only care about buffer dimension resources when looking at texel buffers
                continue;
            };

            uint8_t decorationSet = spirvCompiler.get_decoration(resource.id, spv::DecorationDescriptorSet);
            uint8_t decorationBinding = spirvCompiler.get_decoration(resource.id, spv::DecorationBinding);
            uint8_t decorationLocation = spirvCompiler.get_decoration(resource.id, spv::DecorationLocation); 

            WdUniformAddress address(decorationSet, decorationBinding, decorationLocation); 

            if (_uniforms.find(address) == _uniforms.end()) {
                // not found, add entry 
                WdUniform uniform{}; 
                uniform.paramType = paramType; 
                uniform.decorationSet = decorationSet;
                uniform.decorationBinding = decorationBinding;
                uniform.decorationLocation = decorationLocation;
                uniform.resourceCount = spirvCompiler.get_type(resource.type_id).array[0]; 
                uniform.resources.resize(uniform.resourceCount); // Already resising here, check SET UNIFORM again 
                uniform.stages = WdStage::Unknown;

                _uniforms[address] = uniform; // TODO: this could be move actually
            }

            _uniforms[address].stages |= stage;

            _uniformAddresses[{resource.name, stage}] = address; 
        };
    };

    void WdPipeline::generateVertexParams() {
        // create compiler object for the SPIR-V bytecode
        // Init SPIRV shader code 
        _spirvVertexShader.spirvWords = reinterpret_cast<const uint32_t*>(_vertexShader.data());
        _spirvVertexShader.wordCount = _vertexShader.size();
        spirv_cross::Compiler spirvCompiler( _spirvVertexShader.spirvWords, _spirvVertexShader.wordCount);
        spirv_cross::ShaderResources resources = spirvCompiler.get_shader_resources();

        // Check all non-subpass input uniforms 
        addUniformDescription(spirvCompiler, resources.sampled_images, WdShaderParameterType::WD_SAMPLED_IMAGE, WdStage::Vertex);
        addUniformDescription(spirvCompiler, resources.separate_images, WdShaderParameterType::WD_TEXTURE_IMAGE, WdStage::Vertex);
        addUniformDescription(spirvCompiler, resources.storage_images, WdShaderParameterType::WD_BUFFER_IMAGE, WdStage::Vertex);
        addUniformDescription(spirvCompiler, resources.sampled_images, WdShaderParameterType::WD_SAMPLED_BUFFER, WdStage::Vertex);
        addUniformDescription(spirvCompiler, resources.separate_samplers, WdShaderParameterType::WD_SAMPLER, WdStage::Vertex);
        addUniformDescription(spirvCompiler, resources.uniform_buffers, WdShaderParameterType::WD_UNIFORM_BUFFER, WdStage::Vertex);
        //addUniformDescription(push_constant_buffers, WD_PUSH_CONSTANT); TODO: These need to be handled separately 
        addUniformDescription(spirvCompiler, resources.storage_buffers, WdShaderParameterType::WD_STORAGE_BUFFER, WdStage::Vertex);

        // process vertex input now 
        for (const spirv_cross::Resource &resource : resources.stage_inputs) {
            WdVertexInput vertexInput;
            vertexInput.paramType = WdShaderParameterType::WD_STAGE_INPUT;
            vertexInput.decorationBinding = spirvCompiler.get_decoration(resource.id, spv::DecorationBinding); 
            vertexInput.decorationLocation = spirvCompiler.get_decoration(resource.id, spv::DecorationLocation); 
            vertexInput.offset = spirvCompiler.get_decoration(resource.id, spv::DecorationOffset); 

            const spirv_cross::SPIRType& type = spirvCompiler.get_type(resource.type_id);
            uint32_t vecSize = type.vecsize;
            spirv_cross::SPIRType::BaseType baseType = type.basetype;

            vertexInput.size = vecSize * SPIRVBaseToSize[baseType];
            vertexInput.format = SPIRVtoWdFormat[vecSize - 1][baseType];

            _vertexInputs.push_back(vertexInput);
        };        
    };

    void WdPipeline::generateFragmentParams() {
        _spirvFragmentShader.spirvWords = reinterpret_cast<const uint32_t*>(_vertexShader.data());
        _spirvFragmentShader.wordCount = _vertexShader.size();
        
        // create compiler object for the SPIR-V bytecode
        spirv_cross::Compiler spirvCompiler( _spirvFragmentShader.spirvWords, _spirvFragmentShader.wordCount);
        spirv_cross::ShaderResources resources = spirvCompiler.get_shader_resources();

        // Check all non-subpass input uniforms 
        addUniformDescription(spirvCompiler, resources.sampled_images, WdShaderParameterType::WD_SAMPLED_IMAGE, WdStage::Fragment);
        addUniformDescription(spirvCompiler, resources.separate_images, WdShaderParameterType::WD_TEXTURE_IMAGE, WdStage::Fragment);
        addUniformDescription(spirvCompiler, resources.storage_images, WdShaderParameterType::WD_BUFFER_IMAGE, WdStage::Fragment);
        addUniformDescription(spirvCompiler, resources.sampled_images, WdShaderParameterType::WD_SAMPLED_BUFFER, WdStage::Fragment);
        addUniformDescription(spirvCompiler, resources.separate_samplers, WdShaderParameterType::WD_SAMPLER, WdStage::Fragment);
        addUniformDescription(spirvCompiler, resources.uniform_buffers, WdShaderParameterType::WD_UNIFORM_BUFFER, WdStage::Fragment);
        //addUniformDescription(push_constant_buffers, WD_PUSH_CONSTANT); TODO: These need to be handled separately 
        addUniformDescription(spirvCompiler, resources.storage_buffers, WdShaderParameterType::WD_STORAGE_BUFFER, WdStage::Fragment);

        // process supbass inputs now 
        for (const spirv_cross::Resource &resource : resources.subpass_inputs) {
            WdSubpassInput subpassInput{};
            subpassInput.paramType = WdShaderParameterType::WD_SUBPASS_INPUT;
            subpassInput.decorationBinding = spirvCompiler.get_decoration(resource.id, spv::DecorationBinding); 
            subpassInput.decorationLocation = spirvCompiler.get_decoration(resource.id, spv::DecorationLocation);
            subpassInput.decorationIndex = spirvCompiler.get_decoration(resource.id, spv::DecorationInputAttachmentIndex);
            subpassInput.decorationSet = spirvCompiler.get_decoration(resource.id, spv::DecorationDescriptorSet);
            // by construction the names have to be unique 
            _subpassInputs[resource.name] = subpassInput; 
        };

        // process fragment outputs now 
        for (const spirv_cross::Resource &resource : resources.stage_outputs) {
            WdFragmentOutput fragOutput{};
            fragOutput.paramType = WdShaderParameterType::WD_STAGE_OUTPUT;
            fragOutput.decorationIndex = spirvCompiler.get_decoration(resource.id, spv::DecorationLocation);
            // by construction the names have to be unique 
            _fragmentOutputs[resource.name] = fragOutput; 
        };
    };

}